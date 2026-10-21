#ifndef EDGE_MEMORY_STORE_H
#define EDGE_MEMORY_STORE_H

#include <string>
#include <vector>
#include <memory>
#include <optional>

class StorageManager;
class TextEmbeddingGenerator;

// ============================================================================
// v2 端侧记忆存储（docs/memory_v2_design.md）
// ----------------------------------------------------------------------------
// 与 v1 的 MemoryOrchestrator 的区别：
//   - 用独立的 `memory` 表（无 user/agent/session 外键，避开 P3 那个 40/40 全失败的坑）
//   - 类型收敛到 3 类（semantic/episodic/procedural），preference 并入 semantic
//   - 非破坏性版本化：valid_until / superseded_by / supersedes
//   - 治理三元标签：owner / scope / authority
//   - 显式 CRUD 齐全：update / remove / list / history（v1 全缺）
// Q1 只实现基础 CRUD；五操作消解（CREATE/UPDATE/REINFORCE/SUPERSEDE/NOOP）在 Q2。
// ============================================================================

enum class MemoryType { Semantic, Episodic, Procedural };

// Q1 只会产生 Created / Removed / Failed；其余留给 Q2 的消解逻辑
enum class MemoryAction { Created, Updated, Reinforced, Superseded, Noop, Removed, Failed };

struct Memory {
    int id = 0;
    MemoryType type = MemoryType::Semantic;
    std::string content;
    float strength = 1.0f;
    int accessCount = 0;
    bool valid = true;          // valid_until IS NULL
    int supersededBy = 0;       // 0 = 无
    int supersedes = 0;         // 0 = 无
    std::string owner, scope, authority;
    std::string validFrom, validUntil;
    std::string sourceSession;
    int sourceTurn = 0;
    std::string createdAt, updatedAt;
    float score = 0.f;          // 仅 recall 结果里填
};

struct IngestResult {
    MemoryAction action = MemoryAction::Failed;
    int id = 0;
    float similarity = 0.f;
    std::string reason;
    bool usedLlm = false;
};

struct MemoryStats {
    int total = 0, valid = 0, invalid = 0;
    int semantic = 0, episodic = 0, procedural = 0;
    int dim = 0;
};

const char* memoryTypeName(MemoryType t);
bool memoryTypeFromName(const std::string& s, MemoryType& out);
const char* memoryActionName(MemoryAction a);

class EdgeMemoryStore {
public:
    explicit EdgeMemoryStore(StorageManager& storage);
    ~EdgeMemoryStore();

    bool ready() const { return dim_ > 0; }
    int dim() const { return dim_; }
    const std::string& embedModelTag() const { return embedModelTag_; }

    // ---- 写入（Q1：直接 CREATE；Q2 加五操作消解）----
    // validFrom 留空则用 datetime('now')；Q3 基准载入 episodic 记忆时显式传事件时间，
    // 供 recallTyped 的时间衰减用。
    IngestResult ingestText(const std::string& text, MemoryType type,
                            const std::string& owner = "user",
                            const std::string& scope = "private",
                            const std::string& authority = "asserted",
                            const std::string& sourceSession = "",
                            int sourceTurn = 0,
                            const std::string& validFrom = "");

    // ---- 检索 ----
    std::vector<Memory> recall(const std::string& query, int topK = 4,
                               std::optional<MemoryType> filter = std::nullopt,
                               bool includeHistory = false);

    // ---- Q3：类型特化检索（docs/memory_v2_design.md §7）----
    // detectQueryIntent：关键词判查询意图（优先级 episodic > procedural > semantic）。
    // recallTyped：按意图给同类型记忆加权（×kTypeBoost）+ episodic 时间衰减后重排；
    //   对照基线是 recall()（纯余弦、全类型一起搜、不加权）。
    static MemoryType detectQueryIntent(const std::string& query);
    std::vector<Memory> recallTyped(const std::string& query, int topK = 4,
                                    bool includeHistory = false);
    static constexpr float kTypeBoost = 1.6f;         // 同类型加权系数
    static constexpr float kEpisodicTauDays = 365.f;  // episodic 时间衰减时间常数（天）

    // ---- Q2：供消解器（MemoryResolver）用的无副作用接口 ----
    // embedText：embed() 的公开包装。
    // searchByEmbedding：按给定向量做暴力余弦检索，**不**递增 access_count
    //   （recall 会递增；消解只是比对，不应算作一次真实召回）。
    std::vector<float> embedText(const std::string& text);
    std::vector<Memory> searchByEmbedding(const std::vector<float>& qemb, int topK,
                                          std::optional<MemoryType> filter = std::nullopt,
                                          bool includeHistory = false);

    // ---- 显式 CRUD ----
    bool update(int id, const std::string& newContent);   // 原地改内容 + 重算 embedding
    bool remove(int id);                                  // 物理删除（记录级，非 unlearning）
    bool supersede(int oldId, const std::string& newContent, int* newIdOut = nullptr);  // 非破坏性替换
    bool reinforce(int id, float strengthDelta = 1.0f);   // 等价重述：只提升 strength，不改内容

    // ---- 浏览 ----
    std::vector<Memory> list(std::optional<MemoryType> type = std::nullopt,
                             int limit = 0, bool includeInvalid = false);
    std::vector<Memory> history(int id);   // 沿 supersedes/superseded_by 链回溯整条演化
    Memory get(int id);
    int count(std::optional<MemoryType> type = std::nullopt, bool validOnly = true);
    MemoryStats stats();

private:
    StorageManager& storage_;
    std::unique_ptr<TextEmbeddingGenerator> embedder_;
    int dim_ = 0;
    std::string embedModelTag_;

    std::vector<float> embed(const std::string& text);
};

#endif // EDGE_MEMORY_STORE_H
