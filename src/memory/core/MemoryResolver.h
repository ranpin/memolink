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
};

class MemoryResolver {
public:
    // llm 可为 nullptr：此时 System-2 直接走回退 CREATE（用于离线/无模型测试）
    explicit MemoryResolver(MemoryStore& store, QwenAPI* llm = nullptr);

    // 三层消解：只决策，不写库
    ResolveDecision resolve(const std::string& candidate, MemoryType type);

    // 消解 + 落库
    IngestResult ingest(const std::string& candidate, MemoryType type,
                        const std::string& owner = "user",
                        const std::string& scope = "private",
                        const std::string& authority = "asserted",
                        const std::string& sourceSession = "",
                        int sourceTurn = 0);

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
