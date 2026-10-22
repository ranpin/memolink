#ifndef MEMORY_SYSTEM_H
#define MEMORY_SYSTEM_H

#include <string>
#include <vector>
#include <optional>
#include "EdgeMemoryStore.h"
#include "MemoryResolver.h"
#include "MemoryExtractor.h"

class StorageManager;
class QwenAPI;

// ============================================================================
// 接口层：记忆系统统一对外门面（docs/memory_v2_design.md §11）
// ----------------------------------------------------------------------------
// 应用层（mirix-mem CLI、未来的 HTTP API / agent tool / SDK）只依赖这一个入口，
// 不必知道内部由 EdgeMemoryStore + MemoryResolver + MemoryExtractor 三件套组装、
// 也不必知道它们的构造顺序与依赖。这是「应用可替换、记忆系统统一对外」的落地点：
// 换应用只重写 app/，核心 core/ 与基础设施 infra/ 一行不动。
//
// 生命周期五阶段（与 README / demo 页一致）：
//   ① extract  对话轮 → 候选事实        （MemoryExtractor，受约束 JSON）
//   ② ingest   候选事实 → 五操作 → 落库  （MemoryResolver 三层消解 + EdgeMemoryStore）
//   ④ recall   query → top-k            （EdgeMemoryStore，类型特化）
//   ⑤ update/remove/list/history/stats  （EdgeMemoryStore 显式 CRUD + 版本回溯）
// ============================================================================
class MemorySystem {
public:
    // llm 可为 nullptr：离线/无模型时 System-2 回退 CREATE、extract 返回空。
    // 成员按 store_ → resolver_ → extractor_ 顺序声明，保证 resolver_ 拿到已构造的 store_。
    MemorySystem(StorageManager& storage, QwenAPI* llm);

    bool ready() const { return store_.ready(); }
    int dim() const { return store_.dim(); }
    const std::string& embedModelTag() const { return store_.embedModelTag(); }

    // ① 提取：对话轮 → 候选事实（空数组 = 无可记事实，调用方应 NOOP）
    std::vector<CandidateFact> extract(const std::string& userTurn,
                                       const std::string& assistantTurn);

    // ②③ 消解 + 存储：候选事实 → 五操作之一 → 落库
    IngestResult ingest(const std::string& text, MemoryType type,
                        const std::string& owner = "user",
                        const std::string& scope = "private",
                        const std::string& authority = "asserted",
                        const std::string& sourceSession = "",
                        int sourceTurn = 0);

    // ④ 检索：recall 纯余弦；recallTyped 类型特化（意图路由 + 加权 + 时间衰减）
    std::vector<Memory> recall(const std::string& query, int topK = 4,
                               std::optional<MemoryType> filter = std::nullopt,
                               bool includeHistory = false);
    std::vector<Memory> recallTyped(const std::string& query, int topK = 4,
                                    bool includeHistory = false);

    // ⑤ 显式变更
    bool update(int id, const std::string& newContent);
    bool remove(int id);

    // 浏览
    std::vector<Memory> list(std::optional<MemoryType> type = std::nullopt,
                             int limit = 0, bool includeInvalid = false);
    std::vector<Memory> history(int id);
    Memory get(int id);
    MemoryStats stats();

    // 细粒度入口（评测/高级用法）：直接访问底层 store，不经门面收敛
    EdgeMemoryStore& store() { return store_; }

private:
    EdgeMemoryStore store_;
    MemoryResolver resolver_;
    MemoryExtractor extractor_;
};

#endif // MEMORY_SYSTEM_H
