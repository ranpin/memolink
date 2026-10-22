#ifndef MEMORY_EXTRACTOR_H
#define MEMORY_EXTRACTOR_H

#include <string>
#include <vector>
#include "memory/core/MemoryStore.h"   // MemoryType

class QwenAPI;

// ============================================================================
// Q4/Q6 提取阶段（docs/memory_v2_design.md §6）：对话轮 → 受约束 JSON 候选事实。
// ----------------------------------------------------------------------------
// Mem0 式提取的端侧落地：本地 LLM（非云端）、输出受约束为 JSON 数组
// （每条 {text, type, owner, scope}），不是自由文本。Q4 已用 bench/extract_eval.py
// 实测：受约束 JSON 提取的编造率 3.7%（上界，多为改写非捏造），规避了 P2 实测的
// 开放式生成编造。本类是那个 Python 评测逻辑的 C++ 等价实现，供 CLI/demo 管线用。
//
// 分块：llama-server `-c 4096`，单段长对话一次性喂会 HTTP 400，故按
// kMaxChunkChars 分块逐块提取（同 Q4 extract_eval.py 的 8000 字符分块）。
// ============================================================================

struct CandidateFact {
    std::string text;
    MemoryType type = MemoryType::Semantic;
    std::string owner = "user";    // user | assistant
    std::string scope = "private"; // private | shared
};

class MemoryExtractor {
public:
    explicit MemoryExtractor(QwenAPI* llm) : llm_(llm) {}

    // 从一轮对话（user + assistant）提取候选事实。
    // 返回空数组 = 没有值得记的（寒暄/闲聊），调用方应当 NOOP。
    std::vector<CandidateFact> extract(const std::string& userTurn,
                                       const std::string& assistantTurn);

    static constexpr int kMaxChunkChars = 8000;

private:
    QwenAPI* llm_;

    // 对单个 chunk 调 LLM 提取，解析 JSON 数组。失败返回空。
    std::vector<CandidateFact> extractChunk(const std::string& conversation);
    // 解析 LLM 输出里的 JSON 数组（容错：找第一个平衡的 [...]）。
    static std::vector<CandidateFact> parseFacts(const std::string& out);
};

#endif // MEMORY_EXTRACTOR_H
