#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

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

// Q7：治理三元标签过滤器（docs/memory_v2_design.md §8）。三字段均可选，
// empty() 时不过滤（向后兼容）。检索/列举时按 owner/scope/authority 做访问控制，
// 是「命中 scope=private → 原文不出设备」这条隐私路由的第一道闸。
//
// strict（fail-closed，2026-10-23 加）：默认 false = 空 filter 不过滤、返回全部
// （fail-open，向后兼容，适合单用户 CLI）。true = 空 filter 返回空（fail-closed），
// 堵住「调用方忘传 filter 就读到全部」这个隐患——多租户应用（如座舱助手多驾驶员）
// 应开 strict。注意 strict 只在 filter 为空时改变行为；非空 filter 照常按字段过滤。
struct GovernanceFilter {
    std::optional<std::string> owner;
    std::optional<std::string> scope;
    std::optional<std::string> authority;
    bool strict = false;   // true：空 filter → 返回空（fail-closed），而非返回全部
    bool empty() const { return !owner && !scope && !authority; }
};

class MemoryStore {
public:
    explicit MemoryStore(StorageManager& storage);
    ~MemoryStore();

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
    // gov 为治理过滤器（Q7）：默认空 = 不过滤，向后兼容。
    std::vector<Memory> recall(const std::string& query, int topK = 4,
                               std::optional<MemoryType> filter = std::nullopt,
                               bool includeHistory = false,
                               const GovernanceFilter& gov = {});

    // ---- Q3：类型特化检索（docs/memory_v2_design.md §7）----
    // detectQueryIntent：关键词判查询意图（优先级 episodic > procedural > semantic）。
    // recallTyped：按意图给同类型记忆加权（×kTypeBoost）+ episodic 时间衰减后重排；
    //   对照基线是 recall()（纯余弦、全类型一起搜、不加权）。
    static MemoryType detectQueryIntent(const std::string& query);
    std::vector<Memory> recallTyped(const std::string& query, int topK = 4,
                                    bool includeHistory = false,
                                    const GovernanceFilter& gov = {});
    static constexpr float kTypeBoost = 1.6f;         // 同类型加权系数
    static constexpr float kEpisodicTauDays = 365.f;  // episodic 时间衰减时间常数（天）
    static constexpr float kStrengthTauDays = 90.f;   // Q7 强度衰减时间常数（天）

    // ---- Q2：供消解器（MemoryResolver）用的无副作用接口 ----
    // embedText：embed() 的公开包装。
    // searchByEmbedding：按给定向量做暴力余弦检索，**不**递增 access_count
    //   （recall 会递增；消解只是比对，不应算作一次真实召回）。
    std::vector<float> embedText(const std::string& text);
    std::vector<Memory> searchByEmbedding(const std::vector<float>& qemb, int topK,
                                          std::optional<MemoryType> filter = std::nullopt,
                                          bool includeHistory = false,
                                          const GovernanceFilter& gov = {});

    // ---- 显式 CRUD ----
    bool update(int id, const std::string& newContent);   // 原地改内容 + 重算 embedding
    bool remove(int id);                                  // 物理删除（记录级，非 unlearning）
    bool supersede(int oldId, const std::string& newContent, int* newIdOut = nullptr);  // 非破坏性替换
    bool reinforce(int id, float strengthDelta = 1.0f);   // 等价重述：只提升 strength，不改内容

    // ---- Q7：强度衰减 + 按强度淘汰（docs/memory_v2_design.md §9）----
    // 有效强度 = strength × exp(-age_days / kStrengthTauDays)，age 从 updated_at 算
    // （reinforce/update 会刷新 updated_at ⇒ 被强化的记忆衰减更慢、更耐淘汰）。
    static float effectiveStrength(const Memory& m);
    // 有效记忆数超过 capacity 时，按「类型优先级（episodic 先淘汰 → semantic → procedural
    // 最后）+ 同类型内有效强度最低优先」物理删除最弱者，直到不超过 capacity。返回删除条数。
    // 这是把 v1 的纯 FIFO 淘汰升级成强度淘汰（旁证 RSPM 2610.25561：衰减未强化规则）。
    int evictToCapacity(int capacity);

    // ---- 浏览 ----
    std::vector<Memory> list(std::optional<MemoryType> type = std::nullopt,
                             int limit = 0, bool includeInvalid = false,
                             const GovernanceFilter& gov = {});
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

#endif // MEMORY_STORE_H
