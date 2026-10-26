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
    // vlm 可为 nullptr：未配置 api.vlm_url 时多模态轮次（observe）不可用，文本轮照常。
    CockpitAssistant(MemorySystem& mem, QwenAPI& llm, QwenAPI* vlm = nullptr);

    // 当前驾驶员（治理 owner）。检索时按 (owner=driver) ∪ (scope=shared) 过滤。
    void setDriver(const std::string& driver) { driver_ = driver; }
    const std::string& driver() const { return driver_; }
    bool vlmReady() const { return vlm_ != nullptr; }

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

    // ---- Q9 多模态轮次：图像 → VLM caption → extract → ingest(authority=observed) ----
    struct ObserveResult {
        std::string driver;
        std::string imagePath;      // 视觉来源帧路径（落库到 source_image）
        std::string caption;        // VLM 原始输出（受约束叙述句）
        // Q9 二次确认（E4b 缓解）：对同一帧再做一次中性定向复读，文字/数字字段若复读
        // 不一致（含「看不清」弃读），该事实 authority 从 observed 降为 inferred。
        std::string verification;   // 定向复读的原始输出
        int downgraded = 0;         // 因复读不一致而降为 inferred 的事实数
        std::vector<CandidateFact> extracted;
        std::vector<std::string> factAuthority;  // 与 extracted 一一对应：实际落库的 authority
        std::vector<IngestResult> ingested;
        std::vector<Memory> supersedeChain;   // observed 取代 asserted 时的版本链
    };

    // vlm_ 为 nullptr（未配 api.vlm_url）或图像读不到时返回空 caption、不写库。
    ObserveResult observe(const std::string& imagePath);

private:
    MemorySystem& mem_;
    QwenAPI& llm_;
    QwenAPI* vlm_ = nullptr;   // Q9：VLM 实例（:8098），nullptr = 多模态不可用
    std::string driver_ = "张三";

    // Q9：向 VLM 发一次 (prompt + 图像) 请求，返回文本（失败返回 "Error: ..."）。
    std::string vlmAsk(const std::string& prompt, const std::string& imageDataUrl);

    // 治理感知检索：(owner=driver) ∪ (scope=shared)，去重、按分重排、取 topK。
    // intentOut 回填检测到的类型意图名（semantic/episodic/procedural）。
    std::vector<Memory> recallForDriver(const std::string& query, int topK,
                                        std::string& intentOut);
    // 用召回记忆 + 当前驾驶员 + 用户问题拼出生成 prompt。
    std::string buildPrompt(const std::vector<Memory>& recalled, const std::string& query);
};

#endif  // COCKPIT_ASSISTANT_H
