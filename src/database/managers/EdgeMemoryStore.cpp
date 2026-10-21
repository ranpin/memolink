#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <optional>
#include <sqlite3.h>
#include "EdgeMemoryStore.h"
#include "../core/StorageManager.h"
#include "../initialization/ConfigManager/ConfigManager.h"
#include "../../models/TextEmbeddingGenerator.h"
#include "../../utils/VectorSerializer.h"

// ---------------------------------------------------------------------------
// 类型 / 动作 与字符串互转
// ---------------------------------------------------------------------------
const char* memoryTypeName(MemoryType t) {
    switch (t) {
        case MemoryType::Semantic:    return "semantic";
        case MemoryType::Episodic:    return "episodic";
        case MemoryType::Procedural:  return "procedural";
    }
    return "semantic";
}

bool memoryTypeFromName(const std::string& s, MemoryType& out) {
    if (s == "semantic")   { out = MemoryType::Semantic;   return true; }
    if (s == "episodic")   { out = MemoryType::Episodic;   return true; }
    if (s == "procedural") { out = MemoryType::Procedural; return true; }
    return false;
}

const char* memoryActionName(MemoryAction a) {
    switch (a) {
        case MemoryAction::Created:    return "CREATED";
        case MemoryAction::Updated:    return "UPDATED";
        case MemoryAction::Reinforced: return "REINFORCED";
        case MemoryAction::Superseded: return "SUPERSEDED";
        case MemoryAction::Noop:       return "NOOP";
        case MemoryAction::Removed:    return "REMOVED";
        case MemoryAction::Failed:     return "FAILED";
    }
    return "FAILED";
}

// ---------------------------------------------------------------------------
// 内部工具
// ---------------------------------------------------------------------------
namespace {

float cosineSim(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || a.size() != b.size()) return 0.f;
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    if (na <= 0.f || nb <= 0.f) return 0.f;
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

// 当前 UTC 时间，格式与 SQLite datetime('now') 一致（YYYY-MM-DD HH:MM:SS）。
// ingestText 的 validFrom 留空时用这个，保证与 datetime('now') 同口径。
std::string currentUtcTime() {
    time_t now = time(nullptr);
    struct tm tmv;
    gmtime_r(&now, &tmv);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return std::string(buf);
}

// episodic 时间衰减：exp(-age_days / tau)。越近的事件权重越高（§7「时间衰减加权」）。
// validFrom 形如 "YYYY-MM-DD"（或带时分秒）；解析失败或未来时间一律不衰减（返回 1）。
float episodicDecay(const std::string& validFrom, float tauDays) {
    if (validFrom.size() < 10) return 1.0f;
    struct tm tmv = {};
    if (sscanf(validFrom.c_str(), "%d-%d-%d", &tmv.tm_year, &tmv.tm_mon, &tmv.tm_mday) != 3)
        return 1.0f;
    tmv.tm_year -= 1900;
    tmv.tm_mon  -= 1;
    tmv.tm_isdst = -1;
    const time_t event = mktime(&tmv);
    if (event == static_cast<time_t>(-1)) return 1.0f;
    const double ageDays = difftime(time(nullptr), event) / 86400.0;
    return static_cast<float>(std::exp(-(ageDays < 0 ? 0.0 : ageDays) / tauDays));
}

// SELECT 的列顺序必须与此处下标严格一致
constexpr int COL_ID = 0, COL_TYPE = 1, COL_CONTENT = 2, COL_EMBEDDING = 3,
              COL_STRENGTH = 4, COL_ACCESS = 5, COL_VALID_UNTIL = 6,
              COL_SUPERSEDED_BY = 7, COL_SUPERSEDES = 8, COL_OWNER = 9,
              COL_SCOPE = 10, COL_AUTHORITY = 11, COL_VALID_FROM = 12,
              COL_SOURCE_SESSION = 13, COL_SOURCE_TURN = 14,
              COL_CREATED = 15, COL_UPDATED = 16;

const char* kSelectCols =
    "id, type, content, embedding, strength, access_count, valid_until, "
    "superseded_by, supersedes, owner, scope, authority, valid_from, "
    "source_session, source_turn, created_at, updated_at";

std::string colText(sqlite3_stmt* stmt, int idx) {
    const unsigned char* t = sqlite3_column_text(stmt, idx);
    return t ? reinterpret_cast<const char*>(t) : std::string();
}

Memory rowToMemory(sqlite3_stmt* stmt) {
    Memory m;
    m.id           = sqlite3_column_int(stmt, COL_ID);
    MemoryType ty  = MemoryType::Semantic;
    memoryTypeFromName(colText(stmt, COL_TYPE), ty);
    m.type         = ty;
    m.content      = colText(stmt, COL_CONTENT);
    m.strength     = static_cast<float>(sqlite3_column_double(stmt, COL_STRENGTH));
    m.accessCount  = sqlite3_column_int(stmt, COL_ACCESS);
    m.validUntil   = colText(stmt, COL_VALID_UNTIL);
    m.valid        = m.validUntil.empty();   // valid_until IS NULL ⇒ 当前有效
    m.supersededBy = sqlite3_column_int(stmt, COL_SUPERSEDED_BY);
    m.supersedes   = sqlite3_column_int(stmt, COL_SUPERSEDES);
    m.owner        = colText(stmt, COL_OWNER);
    m.scope        = colText(stmt, COL_SCOPE);
    m.authority    = colText(stmt, COL_AUTHORITY);
    m.validFrom    = colText(stmt, COL_VALID_FROM);
    m.sourceSession= colText(stmt, COL_SOURCE_SESSION);
    m.sourceTurn   = sqlite3_column_int(stmt, COL_SOURCE_TURN);
    m.createdAt    = colText(stmt, COL_CREATED);
    m.updatedAt    = colText(stmt, COL_UPDATED);
    return m;
}

}  // namespace

// ---------------------------------------------------------------------------
// 构造
// ---------------------------------------------------------------------------
EdgeMemoryStore::EdgeMemoryStore(StorageManager& storage)
    : storage_(storage) {
    embedder_ = std::make_unique<TextEmbeddingGenerator>(&storage);
    dim_ = embedder_->getEmbeddingDimension();

    // embed_model tag 取模型文件名，换模型时能据此检出旧 embedding 失配
    const std::string path = ConfigManager::getInstance().getEdgeEmbeddingModelPath();
    if (!path.empty()) {
        const size_t pos = path.find_last_of('/');
        embedModelTag_ = (pos == std::string::npos) ? path : path.substr(pos + 1);
    } else {
        embedModelTag_ = "unset";
    }
}

EdgeMemoryStore::~EdgeMemoryStore() = default;

std::vector<float> EdgeMemoryStore::embed(const std::string& text) {
    if (!embedder_) return {};
    return embedder_->generateTextEmbedding(text);
}

std::vector<float> EdgeMemoryStore::embedText(const std::string& text) {
    return embed(text);
}

// ---------------------------------------------------------------------------
// 写入（Q1：直接 CREATE。Q2 会在这里加五操作消解）
// ---------------------------------------------------------------------------
IngestResult EdgeMemoryStore::ingestText(const std::string& text, MemoryType type,
                                         const std::string& owner, const std::string& scope,
                                         const std::string& authority,
                                         const std::string& sourceSession, int sourceTurn,
                                         const std::string& validFrom) {
    IngestResult result;
    if (text.empty()) {
        result.action = MemoryAction::Failed;
        result.reason = "empty content";
        return result;
    }

    const std::vector<float> emb = embed(text);
    if (emb.empty()) {
        result.action = MemoryAction::Failed;
        result.reason = "embedding failed";
        return result;
    }
    const std::string blob = VectorSerializer::serialize(emb);

    sqlite3* db = storage_.getDatabase();
    if (!db) {
        result.action = MemoryAction::Failed;
        result.reason = "no database handle";
        return result;
    }

    static const char* kSql =
        "INSERT INTO memory (type, content, embedding, embed_model, strength, "
        " owner, scope, authority, valid_from, source_session, source_turn) "
        "VALUES (?, ?, ?, ?, 1.0, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        result.action = MemoryAction::Failed;
        result.reason = std::string("prepare failed: ") + sqlite3_errmsg(db);
        return result;
    }
    const std::string validFromVal = validFrom.empty() ? currentUtcTime() : validFrom;
    sqlite3_bind_text(stmt, 1, memoryTypeName(type), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, embedModelTag_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, scope.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, authority.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, validFromVal.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, sourceSession.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, sourceTurn);

    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!ok) {
        result.action = MemoryAction::Failed;
        result.reason = std::string("insert failed: ") + sqlite3_errmsg(db);
        return result;
    }

    result.action = MemoryAction::Created;
    result.id = static_cast<int>(sqlite3_last_insert_rowid(db));
    result.reason = "created";
    return result;
}

// ---------------------------------------------------------------------------
// 检索：暴力余弦（F2 已证明在 agent 记忆量级下精确检索足够，ANN 是过度设计）
// ---------------------------------------------------------------------------
// 无副作用的相似度检索（不递增 access_count）。recall() 与消解器共用它。
std::vector<Memory> EdgeMemoryStore::searchByEmbedding(const std::vector<float>& qemb, int topK,
                                                       std::optional<MemoryType> filter,
                                                       bool includeHistory) {
    std::vector<Memory> results;
    if (qemb.empty()) return results;

    std::string sql = std::string("SELECT ") + kSelectCols + " FROM memory WHERE 1=1";
    if (!includeHistory) sql += " AND valid_until IS NULL";
    if (filter.has_value()) sql += " AND type = ?";

    sqlite3* db = storage_.getDatabase();
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[EdgeMemoryStore] searchByEmbedding prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return results;
    }
    if (filter.has_value())
        sqlite3_bind_text(stmt, 1, memoryTypeName(*filter), -1, SQLITE_TRANSIENT);

    std::vector<std::pair<float, Memory>> scored;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Memory m = rowToMemory(stmt);
        const int blobSize = sqlite3_column_bytes(stmt, COL_EMBEDDING);
        if (blobSize <= 0) continue;   // 没有 embedding 的行不参与向量检索
        const char* raw = static_cast<const char*>(sqlite3_column_blob(stmt, COL_EMBEDDING));
        const std::vector<float> emb = VectorSerializer::deserialize(std::string(raw, blobSize));
        m.score = cosineSim(qemb, emb);
        scored.emplace_back(m.score, std::move(m));
    }
    sqlite3_finalize(stmt);

    std::sort(scored.begin(), scored.end(),
              [](const std::pair<float, Memory>& a, const std::pair<float, Memory>& b) {
                  return a.first > b.first;
              });

    const size_t n = (topK > 0) ? std::min<size_t>(scored.size(), static_cast<size_t>(topK))
                                : scored.size();
    results.reserve(n);
    for (size_t i = 0; i < n; ++i) results.push_back(scored[i].second);
    return results;
}

std::vector<Memory> EdgeMemoryStore::recall(const std::string& query, int topK,
                                            std::optional<MemoryType> filter,
                                            bool includeHistory) {
    const std::vector<float> qemb = embed(query);
    std::vector<Memory> results = searchByEmbedding(qemb, topK, filter, includeHistory);

    // 命中的记忆 access_count +1（为 Q7 的 LRU/强度衰减留数据）——真实召回的副作用
    sqlite3* db = storage_.getDatabase();
    if (db) {
        sqlite3_stmt* us = nullptr;
        if (sqlite3_prepare_v2(db, "UPDATE memory SET access_count = access_count + 1 WHERE id = ?",
                               -1, &us, nullptr) == SQLITE_OK) {
            for (const auto& m : results) {
                sqlite3_reset(us);
                sqlite3_bind_int(us, 1, m.id);
                sqlite3_step(us);
            }
            sqlite3_finalize(us);
        }
    }
    return results;
}

// ---------------------------------------------------------------------------
// Q3：类型特化检索（docs/memory_v2_design.md §7）
// ---------------------------------------------------------------------------
// 关键词判查询意图，优先级 episodic > procedural > semantic（默认）。
// 刻意不收「怎样」——它在「路线是怎样的」这类描述性问句里是 semantic 意图，
// 收进来会把 semantic 查询误判成 procedural（typed_retrieval.json 的 q16 就是为此改写的）。
MemoryType EdgeMemoryStore::detectQueryIntent(const std::string& query) {
    static const char* kEpisodic[] = {
        "上次", "上一次", "什么时候", "何时", "几天前", "前几天", "上周", "上个星期",
        "昨天", "今天", "今早", "最近", "还记得", "那次", "哪次", "曾经", "之前",
        "上个月", "刚才", "发生过", "遇到"
    };
    static const char* kProcedural[] = {
        "怎么", "如何", "咋", "怎么办", "该怎么", "方法", "步骤", "流程", "教我",
        "做什么", "哪些操作", "该如何"
    };
    for (const char* kw : kEpisodic)
        if (query.find(kw) != std::string::npos) return MemoryType::Episodic;
    for (const char* kw : kProcedural)
        if (query.find(kw) != std::string::npos) return MemoryType::Procedural;
    return MemoryType::Semantic;
}

// 类型特化检索：全类型余弦检索后，按查询意图给同类型记忆加权（×kTypeBoost），
// episodic 再乘时间衰减，然后重排。对照基线是 recall()（纯余弦、不加权）。
std::vector<Memory> EdgeMemoryStore::recallTyped(const std::string& query, int topK,
                                                 bool includeHistory) {
    const MemoryType intent = detectQueryIntent(query);
    const std::vector<float> qemb = embed(query);
    // topK=0 → 取全部，加权重排后再截取
    std::vector<Memory> all = searchByEmbedding(qemb, 0, std::nullopt, includeHistory);

    for (auto& m : all) {
        float w = (m.type == intent) ? kTypeBoost : 1.0f;
        if (m.type == MemoryType::Episodic)
            w *= episodicDecay(m.validFrom, kEpisodicTauDays);
        m.score *= w;
    }
    std::sort(all.begin(), all.end(),
              [](const Memory& a, const Memory& b) { return a.score > b.score; });

    const size_t n = (topK > 0) ? std::min<size_t>(all.size(), static_cast<size_t>(topK))
                                : all.size();
    std::vector<Memory> results(all.begin(), all.begin() + n);

    // 与 recall 一致：命中的记忆 access_count +1
    sqlite3* db = storage_.getDatabase();
    if (db) {
        sqlite3_stmt* us = nullptr;
        if (sqlite3_prepare_v2(db, "UPDATE memory SET access_count = access_count + 1 WHERE id = ?",
                               -1, &us, nullptr) == SQLITE_OK) {
            for (const auto& m : results) {
                sqlite3_reset(us);
                sqlite3_bind_int(us, 1, m.id);
                sqlite3_step(us);
            }
            sqlite3_finalize(us);
        }
    }
    return results;
}

// ---------------------------------------------------------------------------
// 显式 CRUD
// ---------------------------------------------------------------------------
bool EdgeMemoryStore::update(int id, const std::string& newContent) {
    if (newContent.empty()) return false;
    const std::vector<float> emb = embed(newContent);
    if (emb.empty()) return false;
    const std::string blob = VectorSerializer::serialize(emb);

    sqlite3* db = storage_.getDatabase();
    if (!db) return false;

    // 只允许改当前有效的记忆；改历史版本没有意义
    static const char* kSql =
        "UPDATE memory SET content = ?, embedding = ?, embed_model = ?, "
        " updated_at = datetime('now') WHERE id = ? AND valid_until IS NULL;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, newContent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, embedModelTag_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);
    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok && sqlite3_changes(db) > 0;
}

bool EdgeMemoryStore::remove(int id) {
    sqlite3* db = storage_.getDatabase();
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM memory WHERE id = ?", -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(stmt, 1, id);
    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok && sqlite3_changes(db) > 0;
}

// 非破坏性替换：新记录写入并 supersedes=oldId，旧记录标 valid_until + superseded_by。
// 依据 Graphiti 的边失效（32k⭐）；绕开 Mem0 论文 DELETE 分支不插入新事实的缺口。
bool EdgeMemoryStore::supersede(int oldId, const std::string& newContent, int* newIdOut) {
    const Memory old = get(oldId);
    if (old.id == 0) return false;
    if (newContent.empty()) return false;

    const std::vector<float> emb = embed(newContent);
    if (emb.empty()) return false;
    const std::string blob = VectorSerializer::serialize(emb);

    sqlite3* db = storage_.getDatabase();
    if (!db) return false;

    static const char* kInsert =
        "INSERT INTO memory (type, content, embedding, embed_model, strength, owner, scope, "
        " authority, valid_from, supersedes, source_session, source_turn) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kInsert, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, memoryTypeName(old.type), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, newContent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, embedModelTag_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, old.strength);
    sqlite3_bind_text(stmt, 6, old.owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, old.scope.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, old.authority.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, oldId);
    sqlite3_bind_text(stmt, 10, old.sourceSession.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 11, old.sourceTurn);

    const bool inserted = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    if (!inserted) return false;

    const int newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    if (newIdOut) *newIdOut = newId;

    sqlite3_stmt* us = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE memory SET valid_until = datetime('now'), superseded_by = ?, "
            " updated_at = datetime('now') WHERE id = ?", -1, &us, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(us, 1, newId);
    sqlite3_bind_int(us, 2, oldId);
    const bool marked = (sqlite3_step(us) == SQLITE_DONE);
    sqlite3_finalize(us);
    return marked;
}

// 等价重述：只提升 strength（+ 递增 access_count），不改内容、不重算 embedding。
// 依据 Mem0：等价事实不新增记录，只强化已有记忆。只强化当前有效的记忆。
bool EdgeMemoryStore::reinforce(int id, float strengthDelta) {
    sqlite3* db = storage_.getDatabase();
    if (!db) return false;
    static const char* kSql =
        "UPDATE memory SET strength = strength + ?, access_count = access_count + 1, "
        " updated_at = datetime('now') WHERE id = ? AND valid_until IS NULL;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_double(stmt, 1, strengthDelta);
    sqlite3_bind_int(stmt, 2, id);
    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok && sqlite3_changes(db) > 0;
}

// ---------------------------------------------------------------------------
// 浏览
// ---------------------------------------------------------------------------
std::vector<Memory> EdgeMemoryStore::list(std::optional<MemoryType> type, int limit,
                                          bool includeInvalid) {
    std::vector<Memory> results;
    std::string sql = std::string("SELECT ") + kSelectCols + " FROM memory WHERE 1=1";
    if (!includeInvalid) sql += " AND valid_until IS NULL";
    if (type.has_value()) sql += " AND type = ?";
    sql += " ORDER BY id DESC";
    if (limit > 0) sql += " LIMIT " + std::to_string(limit);

    sqlite3* db = storage_.getDatabase();
    if (!db) return results;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;
    if (type.has_value())
        sqlite3_bind_text(stmt, 1, memoryTypeName(*type), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) results.push_back(rowToMemory(stmt));
    sqlite3_finalize(stmt);
    return results;
}

Memory EdgeMemoryStore::get(int id) {
    Memory result;
    sqlite3* db = storage_.getDatabase();
    if (!db) return result;
    const std::string sql = std::string("SELECT ") + kSelectCols + " FROM memory WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) result = rowToMemory(stmt);
    sqlite3_finalize(stmt);
    return result;
}

// 沿 supersedes 链回溯到根，再沿 superseded_by 链正向走完整条演化
std::vector<Memory> EdgeMemoryStore::history(int id) {
    std::vector<Memory> chain;
    Memory cur = get(id);
    if (cur.id == 0) return chain;

    // 回溯到最早的版本
    int guard = 0;
    while (cur.supersedes != 0 && guard++ < 1000) {
        Memory prev = get(cur.supersedes);
        if (prev.id == 0) break;
        cur = prev;
    }
    // 从根正向收集
    guard = 0;
    int walk = cur.id;
    while (walk != 0 && guard++ < 1000) {
        Memory m = get(walk);
        if (m.id == 0) break;
        chain.push_back(m);
        walk = m.supersededBy;
    }
    return chain;
}

int EdgeMemoryStore::count(std::optional<MemoryType> type, bool validOnly) {
    std::string sql = "SELECT COUNT(*) FROM memory WHERE 1=1";
    if (validOnly) sql += " AND valid_until IS NULL";
    if (type.has_value()) sql += " AND type = ?";

    sqlite3* db = storage_.getDatabase();
    if (!db) return 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return 0;
    if (type.has_value())
        sqlite3_bind_text(stmt, 1, memoryTypeName(*type), -1, SQLITE_TRANSIENT);
    int n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

MemoryStats EdgeMemoryStore::stats() {
    MemoryStats s;
    s.total      = count(std::nullopt, false);
    s.valid      = count(std::nullopt, true);
    s.invalid    = s.total - s.valid;
    s.semantic   = count(MemoryType::Semantic, true);
    s.episodic   = count(MemoryType::Episodic, true);
    s.procedural = count(MemoryType::Procedural, true);
    s.dim        = dim_;
    return s;
}
