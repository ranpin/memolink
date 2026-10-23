#include "app/cockpit/CockpitAssistant.h"
#include "memory/api/MemorySystem.h"
#include "infra/llm/QwenAPI.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// 生成回复用的模型名：刻意不含 "omni"（QwenAPI 见到 omni 会切流式解析）。
// llama-server 实际忽略该名、只用已加载的 qwen3-4b，这里只为避开流式分支。
namespace {
constexpr const char* kChatModel = "qwen3-4b";

// 语料 v1 类型 → v2 三类：preference/fact 并入 semantic，episodic/procedural 保留。
MemoryType mapCorpusType(const std::string& t) {
    if (t == "episodic")   return MemoryType::Episodic;
    if (t == "procedural") return MemoryType::Procedural;
    return MemoryType::Semantic;   // preference / fact
}
}  // namespace

CockpitAssistant::CockpitAssistant(MemorySystem& mem, QwenAPI& llm)
    : mem_(mem), llm_(llm) {}

// ---------------------------------------------------------------------------
// 载入语料：直接 CREATE（不走消解——语料是已知事实），validFrom=ts 供 episodic
// 时间衰减；sensitive→scope=private，否则 shared；owner=主驾驶员。
// ---------------------------------------------------------------------------
int CockpitAssistant::seedFromCorpus(const std::string& jsonPath, const std::string& owner) {
    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        std::cerr << "[CockpitAssistant] 打不开语料: " << jsonPath << std::endl;
        return 0;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    Json::Value root;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream is(ss.str());
    if (!Json::parseFromStream(rb, is, &root, &errs) || !root.isObject()) {
        std::cerr << "[CockpitAssistant] 语料 JSON 解析失败: " << errs << std::endl;
        return 0;
    }
    const Json::Value& memories = root["memories"];
    if (!memories.isArray()) {
        std::cerr << "[CockpitAssistant] 语料缺少 memories 数组" << std::endl;
        return 0;
    }

    int loaded = 0;
    for (const auto& item : memories) {
        const std::string text = item.get("text", "").asString();
        if (text.empty()) continue;
        const MemoryType type = mapCorpusType(item.get("type", "fact").asString());
        const bool sensitive = item.get("sensitive", false).asBool();
        const std::string scope = sensitive ? "private" : "shared";
        const std::string ts = item.get("ts", "").asString();
        const int srcTurn = item.get("id", 0).asInt();

        IngestResult r = mem_.store().ingestText(text, type, owner, scope,
                                                 "asserted", "cockpit-corpus", srcTurn, ts);
        if (r.action == MemoryAction::Created) ++loaded;
    }
    return loaded;
}

// ---------------------------------------------------------------------------
// 治理感知检索：(owner=driver) ∪ (scope=shared)。GovernanceFilter 是 AND 语义，
// 无法单条表达「自己的 OR 共享」，故两路各查再合并去重、按分重排、截 topK。
// ---------------------------------------------------------------------------
std::vector<Memory> CockpitAssistant::recallForDriver(const std::string& query, int topK,
                                                     std::string& intentOut) {
    intentOut = memoryTypeName(MemoryStore::detectQueryIntent(query));

    GovernanceFilter own;
    own.owner = driver_;
    GovernanceFilter shared;
    shared.scope = std::string("shared");

    // 每路多取一些候选，合并后再截 topK，跨路合并的 top-k 更准
    const int perPath = topK * 2;
    const std::vector<Memory> a = mem_.recallTyped(query, perPath, /*includeHistory=*/false, own);
    const std::vector<Memory> b = mem_.recallTyped(query, perPath, /*includeHistory=*/false, shared);

    std::vector<Memory> merged;
    std::vector<int> seenIds;
    merged.reserve(a.size() + b.size());
    auto append = [&](const std::vector<Memory>& v) {
        for (const auto& m : v) {
            if (std::find(seenIds.begin(), seenIds.end(), m.id) == seenIds.end()) {
                seenIds.push_back(m.id);
                merged.push_back(m);
            }
        }
    };
    append(a);
    append(b);

    std::sort(merged.begin(), merged.end(),
              [](const Memory& x, const Memory& y) { return x.score > y.score; });
    if (static_cast<int>(merged.size()) > topK) merged.resize(topK);
    return merged;
}

// ---------------------------------------------------------------------------
// 生成 prompt：召回记忆 + 当前驾驶员 + 用户问题。强约束「记忆里没有就是没有」。
// ---------------------------------------------------------------------------
std::string CockpitAssistant::buildPrompt(const std::vector<Memory>& recalled,
                                          const std::string& query) {
    std::ostringstream p;
    p << "/no_think\n"
      << "你是车载座舱语音助手。根据【已知记忆】和驾驶员的话，自然、简短地回应。\n"
      << "规则：\n"
      << "1. 驾驶员提问时：只依据【已知记忆】回答，记忆里没有的就说不知道或没权限，绝不编造，"
      << "也不要用无关记忆推测或假装能完成（如不知道地址就不要假装能导航）。\n"
      << "2. 驾驶员陈述新信息或偏好（不是提问）时：简短自然地确认即可，不要硬套不相关的记忆。\n"
      << "3. 只用一两句口语，不要罗列条目，不要输出「记忆/semantic」等词或任何标签前缀。\n\n"
      << "示例一（提问，记忆里有答案）：\n"
      << "【已知记忆】\n- 上次充电是在公司楼下快充站，充到 90%。\n"
      << "【驾驶员说】我上次充电充到多少？\n"
      << "【助手回答】上次在公司楼下快充站充到了 90%。\n\n"
      << "示例二（陈述新信息）：\n"
      << "【已知记忆】\n- 用户喜欢香氛系统的海洋香型。\n"
      << "【驾驶员说】我开车的时候喜欢喝美式咖啡。\n"
      << "【助手回答】好的，记住您开车时喜欢喝美式咖啡。\n\n"
      << "示例三（提问，但记忆里没有能回答的信息）：\n"
      << "【已知记忆】\n- 车辆软件版本 v3.2.1。\n"
      << "【驾驶员说】我家的地址在哪？\n"
      << "【助手回答】抱歉，我这边没有您家庭地址的记录。\n\n"
      << "现在：\n【已知记忆】\n";
    if (recalled.empty()) {
        p << "（无）\n";
    } else {
        for (const auto& m : recalled)
            p << "- " << m.content << "\n";
    }
    p << "【驾驶员说】" << query << "\n"
      << "【助手回答】";
    return p.str();
}

// ---------------------------------------------------------------------------
// 一轮对话：④ 检索（治理感知）→ 生成本地回复 → ①提取 + ②③消解存储。
// ---------------------------------------------------------------------------
CockpitAssistant::TurnResult CockpitAssistant::respond(const std::string& userText, int topK) {
    TurnResult r;
    r.driver = driver_;
    r.userText = userText;

    // ④ 检索（治理感知）
    r.recalled = recallForDriver(userText, topK, r.intent);

    // demo 标注：无过滤召回一遍，标出对当前驾驶员不可见的私人记忆（治理拦截证据）
    {
        const std::vector<Memory> unfiltered =
            mem_.recallTyped(userText, topK, /*includeHistory=*/false, GovernanceFilter{});
        for (const auto& m : unfiltered) {
            const bool visible =
                std::find_if(r.recalled.begin(), r.recalled.end(),
                             [&](const Memory& v) { return v.id == m.id; }) != r.recalled.end();
            if (!visible && m.owner != driver_ && m.scope != "shared")
                r.blocked.push_back(m);
        }
    }

    // 生成回复（本地 LLM）
    {
        QwenChatCompletionRequest req;
        req.model = kChatModel;
        req.temperature = 0.7f;
        req.top_p = 0.9f;
        req.max_tokens = 256;
        QwenMessage msg;
        msg.role = "user";
        msg.content = buildPrompt(r.recalled, userText);
        req.messages.push_back(msg);
        const QwenChatCompletionResponse resp = llm_.chat_completion(req);
        r.response = resp.content;
    }

    // ① 提取 + ②③ 消解存储（写路径）；owner=当前驾驶员，scope 取提取结果（默认 private）。
    // 只从【用户话】提取，不喂助手回复：回复是由召回记忆生成的，若从中提取会把刚召回的
    // 内容又写回去（循环），还会让纯查询轮误写入。新信息只在用户话里。
    r.extracted = mem_.extract(userText, "");
    for (const auto& f : r.extracted) {
        std::string scope = f.scope;
        if (scope != "private" && scope != "shared") scope = "private";
        const IngestResult ir =
            mem_.ingest(f.text, f.type, driver_, scope, "asserted", "cockpit-chat", 0);
        r.ingested.push_back(ir);
        if (ir.action == MemoryAction::Superseded && r.supersedeChain.empty())
            r.supersedeChain = mem_.history(ir.id);
    }
    return r;
}
