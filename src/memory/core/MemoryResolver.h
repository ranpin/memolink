#ifndef MEMORY_RESOLVER_H
#define MEMORY_RESOLVER_H

#include <string>
#include <vector>
#include "memory/core/MemoryStore.h"

class QwenAPI;

// ============================================================================
// Q2 五操作消解（docs/memory_v2_design.md §5 三层消解）
// ----------------------------------------------------------------------------
// System-1（确定性，不调模型）：
//   score < θ_new(0.50)  → CREATE
//   score ≥ θ_dup(0.999) → REINFORCE（近逐字重复）
// System-2（模糊地带 θ_new ≤ score < θ_dup 调本地 LLM）：
//   约束输出四选一 {UPDATE, SUPERSEDE, REINFORCE, NOOP} + target_id
// 回退（LLM 不可用/超时/解析失败）：CREATE（绝不覆盖或丢失旧记忆）
//
// 标定依据（2026-10-21，dog，bge-small INT8，30 场景，bench/calibrate_thresholds.py）：
//   CREATE 最高相似度 0.442 < SUPERSEDE 最低 0.556 ⇒ θ_new=0.50 可干净切出 CREATE；
//   UPDATE/SUPERSEDE/REINFORCE 全挤在 0.77–1.0，且新颖度信号（new_ratio）也分不开
//   （UPDATE 中位 0.448 vs SUPERSEDE 0.477 几乎全叠）⇒ 这三类必须交给 System-2。
//   System-1 实测覆盖率 ≈32%（7 CREATE + 1 逐字 REINFORCE / 25 可记）。
//
// NOOP 归属：寒暄/闲聊类候选相似度低（会落进 System-1 的 CREATE 区间），真正的
//   NOOP 过滤是提取阶段（Q4）的职责；System-2 保留 NOOP 出口作为模糊地带的兜底。
// ============================================================================

struct ResolveDecision {
    MemoryAction action = MemoryAction::Created;
    int targetId = 0;          // UPDATE/REINFORCE/SUPERSEDE 的目标记忆 id
    float similarity = 0.f;    // 与最相似已有记忆的余弦分数
    bool usedLlm = false;      // 是否走了 System-2
    std::string reason;        // 判定理由 / LLM 原始输出（调试用）
    // ---- Q9 authority 闸门（docs/q9_multimodal_plan.md §1.4）----
    bool blockedByAuthority = false;   // 低可信度候选被拦下 → 改为并存 CREATE
    int blockedTargetId = 0;           // 被拦下时原本要操作的高可信度记忆 id
    bool authorityPromoted = false;    // UPDATE 因候选 authority 更高而升级为 SUPERSEDE
    // 过闸门前 System-1/2 的原始判定。M2 的 before/after 对照用它当「authority-blind
    // 基线会做什么」；座舱 demo 展示「本想 SUPERSEDE 但被 authority 拦下」也靠它。
    MemoryAction preGateAction = MemoryAction::Created;
};

class MemoryResolver {
public:
    // llm 可为 nullptr：此时 System-2 直接走回退 CREATE（用于离线/无模型测试）
    explicit MemoryResolver(MemoryStore& store, QwenAPI* llm = nullptr);

    // 三层消解：只决策，不写库。authority = 候选记忆的来源可信度（Q9 闸门用）。
    ResolveDecision resolve(const std::string& candidate, MemoryType type,
                            const std::string& authority = "asserted");

    // 消解 + 落库。sourceImage = Q9 视觉来源帧路径（caption-then-embed 的溯源附件）。
    IngestResult ingest(const std::string& candidate, MemoryType type,
                        const std::string& owner = "user",
                        const std::string& scope = "private",
                        const std::string& authority = "asserted",
                        const std::string& sourceSession = "",
                        int sourceTurn = 0,
                        const std::string& sourceImage = "");

    // ---- Q9：authority 参与消解决策（此前 authority 只用于治理过滤，从不参与消解）----
    // 优先级 observed > asserted > inferred；未知值按 asserted 处理。
    static int authorityRank(const std::string& authority);

    // authority 闸门。**刻意做成纯函数 + public static**：无 LLM 时 System-2 回退 CREATE，
    // 闸门逻辑本身必须能离线独立单测（test_multimodal_authority）。
    // 只在候选与目标 authority **不同**时介入：
    //   候选更高 → UPDATE 升级为 SUPERSEDE（观察取代口述，建版本链）
    //   候选更低 → SUPERSEDE/UPDATE 降级为 CREATE（入库但不建取代关系，两条并存）
    //   相同     → 完全不介入（这是 Q2 五操作行为逐条不变的保证，M3 硬闸门）
    static void applyAuthorityGate(ResolveDecision& d, const Memory& target,
                                   const std::string& candAuthority);

    // 标定阈值（docs/memory_v2_design.md §5，从实测分布推导，勿拍脑袋改）
    static constexpr float kThetaNew = 0.50f;
    static constexpr float kThetaDup = 0.999f;
    static constexpr int   kSystem2TopK = 3;   // 喂给 System-2 的候选记忆条数

private:
    MemoryStore& store_;
    QwenAPI* llm_;

    ResolveDecision runSystem2(const std::string& candidate,
                               const std::vector<Memory>& matches);
};

#endif // MEMORY_RESOLVER_H
