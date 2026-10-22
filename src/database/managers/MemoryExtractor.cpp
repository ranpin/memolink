#include "MemoryExtractor.h"
#include "../../models/QwenAPI.h"
#include <json/json.h>
#include <sstream>
#include <iostream>

// Q4 实测有效的提取 prompt（bench/extract_eval.py 同款）：受约束 JSON 输出、
// 明确禁止编造、跳过寒暄。{{ }} 是字面花括号（本 prompt 不走 std::format）。
static const char* kExtractPromptTemplate = R"(You are the memory-extraction module of an on-device agent. Extract persistent, reusable facts from the conversation.

Strict rules:
- Output ONLY a JSON array, no prose, no code fences.
- Each element: {"text": "<one self-contained fact>", "type": "semantic|episodic|procedural", "owner": "user|assistant", "scope": "private|shared"}
- type: semantic = facts/preferences/attributes; episodic = past events (keep any date/time); procedural = how-to / skills / routines.
- Extract ONLY what is explicitly stated. NEVER invent, infer, or add any detail not present in the conversation.
- Skip small talk, questions, and filler. If nothing is worth remembering, output [].

Conversation:
{conversation}

JSON array:)";

static MemoryType typeFromString(const std::string& s) {
    if (s == "episodic")   return MemoryType::Episodic;
    if (s == "procedural") return MemoryType::Procedural;
    return MemoryType::Semantic;
}

std::vector<CandidateFact> MemoryExtractor::extract(const std::string& userTurn,
                                                    const std::string& assistantTurn) {
    std::vector<CandidateFact> all;
    if (!llm_) return all;

    // 拼对话（与 Q4 extract_eval.py 的 session_to_text 同格式）
    std::string conv;
    if (!userTurn.empty())      conv += "user: " + userTurn + "\n";
    if (!assistantTurn.empty()) conv += "assistant: " + assistantTurn + "\n";
    if (conv.empty()) return all;

    // 分块（kMaxChunkChars）。单轮对话通常 < 8000 字符，但长 assistant 回复可能超。
    std::vector<std::string> chunks;
    if ((int)conv.size() <= kMaxChunkChars) {
        chunks.push_back(conv);
    } else {
        // 按行切块，尽量不切断一句话
        std::istringstream iss(conv);
        std::string line, cur;
        int curLen = 0;
        while (std::getline(iss, line)) {
            if (curLen + (int)line.size() + 1 > kMaxChunkChars && !cur.empty()) {
                chunks.push_back(cur);
                cur.clear();
                curLen = 0;
            }
            cur += line + "\n";
            curLen += (int)line.size() + 1;
        }
        if (!cur.empty()) chunks.push_back(cur);
    }

    for (const auto& chunk : chunks) {
        auto facts = extractChunk(chunk);
        all.insert(all.end(), facts.begin(), facts.end());
    }
    return all;
}

std::vector<CandidateFact> MemoryExtractor::extractChunk(const std::string& conversation) {
    std::vector<CandidateFact> facts;
    if (!llm_) return facts;

    // 替换 {conversation} 占位符
    std::string prompt = kExtractPromptTemplate;
    const std::string placeholder = "{conversation}";
    const size_t pos = prompt.find(placeholder);
    if (pos != std::string::npos) prompt.replace(pos, placeholder.size(), conversation);

    QwenChatCompletionRequest req;
    req.model = "qwen3-4b";   // llama-server 忽略该名，只为避开 omni 流式分支
    req.messages.push_back(QwenMessage{"user", prompt, {}});
    req.temperature = 0.0f;
    req.top_p = 1.0f;
    req.max_tokens = 1024;

    QwenChatCompletionResponse resp;
    try {
        resp = llm_->chat_completion(req);
    } catch (...) {
        std::cerr << "[MemoryExtractor] LLM 调用失败\n";
        return facts;
    }
    if (resp.content.rfind("Error:", 0) == 0) {
        std::cerr << "[MemoryExtractor] LLM 返回错误: " << resp.content << "\n";
        return facts;
    }
    return parseFacts(resp.content);
}

std::vector<CandidateFact> MemoryExtractor::parseFacts(const std::string& out) {
    std::vector<CandidateFact> facts;
    // 找第一个平衡的 [...]（容错：LLM 可能在 JSON 前后加文字）
    const size_t start = out.find('[');
    if (start == std::string::npos) return facts;
    int depth = 0;
    size_t end = std::string::npos;
    for (size_t i = start; i < out.size(); ++i) {
        if (out[i] == '[') ++depth;
        else if (out[i] == ']') {
            --depth;
            if (depth == 0) { end = i; break; }
        }
    }
    if (end == std::string::npos) return facts;

    Json::Value arr;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream is(out.substr(start, end - start + 1));
    if (!Json::parseFromStream(rb, is, &arr, &errs) || !arr.isArray()) return facts;

    for (const auto& e : arr) {
        if (!e.isObject() || !e.isMember("text")) continue;
        CandidateFact f;
        f.text = e["text"].asString();
        if (f.text.empty()) continue;
        f.type = typeFromString(e.get("type", "semantic").asString());
        f.owner = e.get("owner", "user").asString();
        f.scope = e.get("scope", "private").asString();
        facts.push_back(std::move(f));
    }
    return facts;
}
