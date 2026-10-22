#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <sqlite3.h>
#include <json/json.h>
#include "legacy/core-v1/MemoryOrchestrator.h"

// ---------------------------------------------------------------------------
// 构造：embedder_ 用 TextEmbeddingGenerator（config.ini [edge] 配上路径即走本地 ONNX）
// ---------------------------------------------------------------------------
MemoryOrchestrator::MemoryOrchestrator(StorageManager& storage, ModelManager& model)
    : storage(storage),
      model(model),
      recallMemoryManager(storage) {
    embedder_ = std::make_unique<TextEmbeddingGenerator>(&storage);
    ensureDefaultEntities();
}

// ---------------------------------------------------------------------------
// recall_memory 对 user / agent / session 三张表都有外键，而
// StorageManager::initialize() 执行了 PRAGMA foreign_keys = ON。
// 本记忆系统不使用多用户/多 agent/多会话的关系结构，统一挂在一组默认实体上——
// 这里幂等地把它们建出来，否则每条写入都会外键违约（P3 首跑 40/40 全失败即此因）。
// ---------------------------------------------------------------------------
void MemoryOrchestrator::ensureDefaultEntities() {
    storage.executeQuery("INSERT OR IGNORE INTO user (id, name) VALUES (1, 'default');");
    storage.executeQuery("INSERT OR IGNORE INTO agent (id, user_id, name) VALUES (1, 1, 'default');");
    storage.executeQuery(
        "INSERT OR IGNORE INTO session (id, user_id, agent_id, status) VALUES (1, 1, 1, 'active');");
}

// ---------------------------------------------------------------------------
// 工具：从 metadata JSON 里解析 sensitive 标记
// ---------------------------------------------------------------------------
bool MemoryOrchestrator::parseSensitive(const std::string& metadata) const {
    if (metadata.empty()) return false;
    Json::Value root;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream ms(metadata);
    if (!Json::parseFromStream(rb, ms, &root, &errs)) return false;
    return root.get("sensitive", false).asBool();
}

// ---------------------------------------------------------------------------
// 写入：编码 → 落库（message_type=type）→ 存 embedding → 按类型淘汰
// ---------------------------------------------------------------------------
bool MemoryOrchestrator::write(const std::string& type, const std::string& content,
                               bool sensitive, const std::string& metadata) {
    std::vector<float> emb = embedder_->generateTextEmbedding(content);
    if (emb.empty()) {
        std::cerr << "[Orchestrator] embedding 失败，拒绝写入: " << content << std::endl;
        return false;
    }

    // 把 sensitive 合并进 metadata（保留调用方传入的其它字段，如 ts）
    Json::Value meta;
    if (!metadata.empty()) {
        Json::CharReaderBuilder rb;
        std::string errs;
        std::istringstream ms(metadata);
        Json::parseFromStream(rb, ms, &meta, &errs);  // best-effort，失败则 meta 为空对象
    }
    meta["sensitive"] = sensitive;
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    const std::string meta_str = Json::writeString(wb, meta);

    RecallMemoryManager::RecallMemoryItem item;
    item.id = 0;
    item.user_id = user_id_;
    item.agent_id = agent_id_;
    item.session_id = session_id_;
    item.message_type = type;   // 记忆类型存进 message_type
    item.content = content;
    item.metadata = meta_str;

    if (!recallMemoryManager.add_recall_memory(item)) {
        std::cerr << "[Orchestrator] add_recall_memory 失败" << std::endl;
        return false;
    }

    // add_recall_memory 只返回 bool，新 id 用 sqlite3_last_insert_rowid 取
    sqlite3* db = storage.getDatabase();
    if (!db) return false;
    const int new_id = static_cast<int>(sqlite3_last_insert_rowid(db));

    if (!recallMemoryManager.saveEmbedding(new_id, emb)) {
        std::cerr << "[Orchestrator] saveEmbedding 失败 id=" << new_id << std::endl;
        return false;
    }

    evictType(type);
    return true;
}

// ---------------------------------------------------------------------------
// 检索：编码查询 → searchByEmbedding → 重算余弦（它不返回分数）→ 按分排序
// ---------------------------------------------------------------------------
std::vector<MemoryHit> MemoryOrchestrator::retrieve(const std::string& query, int topK) {
    std::vector<MemoryHit> hits;
    std::vector<float> qemb = embedder_->generateTextEmbedding(query);
    if (qemb.empty()) return hits;

    auto items = recallMemoryManager.searchByEmbedding(user_id_, qemb, topK);
    hits.reserve(items.size());
    for (const auto& item : items) {
        std::vector<float> memb = recallMemoryManager.getEmbedding(item.id);
        float score = 0.f;
        if (!memb.empty() && memb.size() == qemb.size()) {
            float dot = 0.f, na = 0.f, nb = 0.f;
            for (size_t i = 0; i < qemb.size(); ++i) {
                dot += qemb[i] * memb[i];
                na += qemb[i] * qemb[i];
                nb += memb[i] * memb[i];
            }
            if (na > 0.f && nb > 0.f) score = dot / (std::sqrt(na) * std::sqrt(nb));
        }
        MemoryHit h;
        h.id = item.id;
        h.type = item.message_type;
        h.content = item.content;
        h.score = score;
        h.sensitive = parseSensitive(item.metadata);
        h.timestamp = item.created_at;
        hits.push_back(h);
    }
    std::sort(hits.begin(), hits.end(),
              [](const MemoryHit& a, const MemoryHit& b) { return a.score > b.score; });
    return hits;
}

// ---------------------------------------------------------------------------
// 分类型有界淘汰：超限按 id（=写入顺序，FIFO）删最旧
// ---------------------------------------------------------------------------
int MemoryOrchestrator::evictType(const std::string& type) {
    auto it = type_capacity_.find(type);
    const int cap = (it != type_capacity_.end()) ? it->second : default_capacity_;
    if (cap <= 0) return 0;  // <=0 视为不限

    auto items = recallMemoryManager.list_recall_memories(user_id_, 0);
    std::vector<int> ids;
    for (const auto& item : items)
        if (item.message_type == type) ids.push_back(item.id);

    std::sort(ids.begin(), ids.end());  // id 升序 = 写入顺序，最旧在前
    const int excess = static_cast<int>(ids.size()) - cap;
    if (excess <= 0) return 0;

    int evicted = 0;
    for (int i = 0; i < excess; ++i) {
        const std::string sql = "DELETE FROM recall_memory WHERE id = " + std::to_string(ids[i]);
        if (storage.executeQuery(sql)) ++evicted;
    }
    if (evicted > 0)
        std::cout << "[Orchestrator] 淘汰 type=" << type << " " << evicted
                  << " 条（容量 " << cap << "）" << std::endl;
    return evicted;
}

int MemoryOrchestrator::evict() {
    auto items = recallMemoryManager.list_recall_memories(user_id_, 0);
    std::map<std::string, int> seen;
    for (const auto& item : items) seen[item.message_type] = 1;
    int total = 0;
    for (const auto& kv : seen) total += evictType(kv.first);
    return total;
}

// ---------------------------------------------------------------------------
// 从 JSON 语料加载种子记忆（格式见 bench/corpus/*.json）
// ---------------------------------------------------------------------------
int MemoryOrchestrator::loadCorpus(const std::string& corpusPath) {
    std::ifstream f(corpusPath);
    if (!f.is_open()) {
        std::cerr << "[Orchestrator] 无法打开语料: " << corpusPath << std::endl;
        return 0;
    }
    Json::Value root;
    Json::CharReaderBuilder rb;
    std::string errs;
    if (!Json::parseFromStream(rb, f, &root, &errs)) {
        std::cerr << "[Orchestrator] 语料 JSON 解析失败: " << errs << std::endl;
        return 0;
    }
    const Json::Value& memories = root["memories"];
    int count = 0;
    for (const auto& m : memories) {
        const std::string type = m.get("type", "semantic").asString();
        const std::string text = m.get("text", "").asString();
        const bool sensitive = m.get("sensitive", false).asBool();
        const std::string ts = m.get("ts", "").asString();
        if (text.empty()) continue;

        Json::Value meta;
        meta["ts"] = ts;
        Json::StreamWriterBuilder wb;
        wb["indentation"] = "";
        if (write(type, text, sensitive, Json::writeString(wb, meta))) ++count;
    }
    std::cout << "[Orchestrator] 语料加载完成: " << count << "/" << memories.size() << " 条" << std::endl;
    return count;
}

void MemoryOrchestrator::setTypeCapacity(const std::string& type, int cap) {
    type_capacity_[type] = cap;
}

int MemoryOrchestrator::countByType(const std::string& type) {
    auto items = recallMemoryManager.list_recall_memories(user_id_, 0);
    int count = 0;
    for (const auto& item : items)
        if (item.message_type == type) ++count;
    return count;
}

int MemoryOrchestrator::countAll() {
    return static_cast<int>(recallMemoryManager.list_recall_memories(user_id_, 0).size());
}

// ---------------------------------------------------------------------------
// 原有四个方法（填充实空壳，保留签名兼容既有调用）
// ---------------------------------------------------------------------------
bool MemoryOrchestrator::processMemory(const std::string& /*userId*/, const std::string& /*sessionId*/,
                                       const std::string& /*agentId*/, const std::string& memoryType,
                                       const std::string& content, const std::string& metadata) {
    return write(memoryType, content, parseSensitive(metadata), metadata);
}

std::string MemoryOrchestrator::retrieveMemory(const std::string& /*userId*/, const std::string& query,
                                               const std::string& /*queryType*/, int topK) {
    auto hits = retrieve(query, topK);
    std::ostringstream oss;
    for (size_t i = 0; i < hits.size(); ++i) {
        oss << (i + 1) << ". [" << hits[i].type << "] " << hits[i].content;
        if (hits[i].sensitive) oss << "（敏感）";
        oss << "\n";
    }
    return oss.str();
}

bool MemoryOrchestrator::add_memory_with_embedding(const std::string& memoryType,
                                                   const std::string& content,
                                                   const std::vector<float>& embedding,
                                                   const std::string& metadata) {
    RecallMemoryManager::RecallMemoryItem item;
    item.id = 0;
    item.user_id = user_id_;
    item.agent_id = agent_id_;
    item.session_id = session_id_;
    item.message_type = memoryType;
    item.content = content;
    item.metadata = metadata.empty() ? "{}" : metadata;

    if (!recallMemoryManager.add_recall_memory(item)) return false;
    sqlite3* db = storage.getDatabase();
    if (!db) return false;
    const int new_id = static_cast<int>(sqlite3_last_insert_rowid(db));
    if (!recallMemoryManager.saveEmbedding(new_id, embedding)) return false;
    evictType(memoryType);
    return true;
}

std::vector<int> MemoryOrchestrator::search_memory_by_embedding(const std::vector<float>& queryEmbedding,
                                                                int topK) {
    auto items = recallMemoryManager.searchByEmbedding(user_id_, queryEmbedding, topK);
    std::vector<int> ids;
    ids.reserve(items.size());
    for (const auto& item : items) ids.push_back(item.id);
    return ids;
}
