#ifndef COCKPIT_ASSISTANT_H
#define COCKPIT_ASSISTANT_H

#include <string>
#include <vector>
#include "memory/core/MemoryStore.h"      // Memory / IngestResult / GovernanceFilter / MemoryType
#include "memory/core/MemoryExtractor.h"  // CandidateFact

class MemorySystem;
class QwenAPI;

// ============================================================================
// 座舱助手 —— app 层第二个公民（第一个是 memolink CLI）
// ----------------------------------------------------------------------------
// 只依赖接口层门面 MemorySystem + QwenAPI，不直接碰 core 三件套；core/infra 一行
// 不动。同一个记忆核驱动两个应用，是「应用可替换、门面稳定」的活证明。
//
// 在座舱场景里点亮 memolink 的独有机制（demo 的 so-what）：
//   - 偏好改变   → SUPERSEDE 非破坏性版本化（旧值失效、新值入库、history 可回溯）
//   - 「上次充电」→ episodic 类型特化检索（意图路由 + 时间衰减）
//   - 多驾驶员   → 治理过滤（他人的 private 记忆对当前驾驶员不可见）
// ============================================================================
class CockpitAssistant {
public:
    CockpitAssistant(MemorySystem& mem, QwenAPI& llm);

    // 当前驾驶员（治理 owner）。检索时按 (owner=driver) ∪ (scope=shared) 过滤。
    void setDriver(const std::string& driver) { driver_ = driver; }
    const std::string& driver() const { return driver_; }

    // 载入语料（bench/corpus/cockpit.json）作为初始记忆库。
    // 映射：type preference/fact→semantic、episodic→episodic、procedural→procedural；
    //       sensitive=true→scope=private，否则 shared；owner=主驾驶员；validFrom=ts
    //       （供 episodic 时间衰减）。直接 CREATE（不走消解，语料是已知事实）。
    // 返回成功载入条数。
    int seedFromCorpus(const std::string& jsonPath, const std::string& owner);

    struct TurnResult {
        std::string driver;
        std::string userText;
        std::string intent;                    // recallTyped 检测的类型意图（demo 标注）
        std::vector<Memory> recalled;          // 治理后可见、用于生成回复的记忆
        std::vector<Memory> blocked;           // 无过滤会命中、但对当前驾驶员不可见（demo 标注）
        std::string response;                  // 本地 LLM 生成的回复
        std::vector<CandidateFact> extracted;  // 提取出的候选事实
        std::vector<IngestResult> ingested;    // 与 extracted 一一对应的消解结果
        std::vector<Memory> supersedeChain;    // 若发生 SUPERSEDE，其版本链（旧→新）
    };

    // 一轮对话：④ 检索（治理感知）→ 生成回复 → ①提取 + ②③消解存储。
    TurnResult respond(const std::string& userText, int topK = 4);

private:
    MemorySystem& mem_;
    QwenAPI& llm_;
    std::string driver_ = "张三";

    // 治理感知检索：(owner=driver) ∪ (scope=shared)，去重、按分重排、取 topK。
    // intentOut 回填检测到的类型意图名（semantic/episodic/procedural）。
    std::vector<Memory> recallForDriver(const std::string& query, int topK,
                                        std::string& intentOut);
    // 用召回记忆 + 当前驾驶员 + 用户问题拼出生成 prompt。
    std::string buildPrompt(const std::vector<Memory>& recalled, const std::string& query);
};

#endif  // COCKPIT_ASSISTANT_H
