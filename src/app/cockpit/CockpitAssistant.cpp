#include "app/cockpit/CockpitAssistant.h"
#include "memory/api/MemorySystem.h"
#include "infra/llm/QwenAPI.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cctype>

// 生成回复用的模型名：刻意不含 "omni"（QwenAPI 见到 omni 会切流式解析）。
// llama-server 实际忽略该名、只用已加载的 qwen3-4b，这里只为避开流式分支。
namespace {
constexpr const char* kChatModel = "qwen3-4b";
// Q9：VLM 模型名，同样刻意不含 "omni"。llama-server 忽略该名、只用已加载的 Qwen3-VL。
constexpr const char* kVlmModel = "qwen3vl";

// Q9：受约束 caption prompt（V4 持久状态式，2026-10-25 在 dog 上对 4 个变体实测选定）。
//   解决 docs/q9_multimodal_plan.md §4 记录的张力，并修掉一个更深的坑：
//
//   🔴 caption 的**语法框架决定提取器判的记忆类型**，而类型决定 authority 闸门能否触发
//   ——resolver 只在**同类型内**比对（searchByEmbedding 带 type 过滤）。驾驶员口述天然
//   被判 semantic，若 caption 是场景描述句（「车内有一个幼儿坐在…」）会被判 episodic，
//   两者永不见面 ⇒ 闸门在真实应用里根本不会触发（实测 sim=0.000、System-1 直接 CREATE）。
//
//   V4 要求「持久状态或固定配置」句式（「车内安装了儿童安全座椅」）⇒ 提取器判 semantic，
//   与口述同类型，闸门才有机会裁决。且 V4 让提取器拆出多条原子事实（座椅图 5 条、
//   仪表盘 4 条），首条与口述几乎同义、相似度够高。
//
//   变体实测：V1 纯描述式（39 tok，冗长视觉细节，提取器原样照搬）；V2 事实式（18 tok，
//   简洁但仍是场景句 → episodic）；V3 两步式（「事实」行会做推断，图里只有牌子却输出
//   「车辆所在位置…」，M4a 编造风险，弃用）；**V4 持久状态式（选定）**。
constexpr const char* kCaptionPrompt =
    "这是座舱摄像头拍摄的画面。请用一句完整的陈述句，记录画面中车辆或乘员的持久状态或固定配置"
    "（例如车内安装或放置了什么设备物品、车辆停放的楼层与车位号、仪表的警告状态）。"
    "不要描述瞬时的动作、人物姿态或场景氛围。要求：\n"
    "① 只陈述画面中确实存在的内容，不要推测画面之外的信息；\n"
    "② 画面中若有文字或数字，必须逐字准确读出并写进句子里；\n"
    "③ 看不清的部分直接说「看不清」，不要猜。";

// Q9 二次确认 prompt（E4b 缓解）。**刻意中性**——不带「座舱摄像头」前提：实测带座舱
// 前提的 prompt 在非座舱画面上会拒答（「画面中不存在座舱摄像头拍摄的车辆或乘员」），
// 那样就拿不到可对比的文字读数了。只要求逐字读文字/数字 + 读不清就弃读。
//
// 为什么这能挡住漂移：实测同一张座椅图，V4 叙述式硬读出「PARENTA」，而中性定向复读
// 一致回答「看不清」⇒ 两者不一致 ⇒ 该文字字段不可信 ⇒ 降为 inferred。
// ⚠️ 已知上限：**挡不住「稳定误读」**——speed20 两种 prompt 都读「2.0」，自一致性
// 原理上抓不到「一致地读错」（见 q9_multimodal_plan.md §8.7）。
constexpr const char* kVerifyPrompt =
    "请只读出这张图片中出现的文字和数字，逐个准确读出，"
    "看不清的写「看不清」，不要描述其它内容。";

// 语料 v1 类型 → v2 三类：preference/fact 并入 semantic，episodic/procedural 保留。
MemoryType mapCorpusType(const std::string& t) {
    if (t == "episodic")   return MemoryType::Episodic;
    if (t == "procedural") return MemoryType::Procedural;
    return MemoryType::Semantic;   // preference / fact
}

std::string base64Encode(const std::string& in) {
    static const char* kTbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < in.size()) {
        const unsigned v = (static_cast<unsigned char>(in[i]) << 16) |
                           (static_cast<unsigned char>(in[i + 1]) << 8) |
                            static_cast<unsigned char>(in[i + 2]);
        out.push_back(kTbl[(v >> 18) & 63]);
        out.push_back(kTbl[(v >> 12) & 63]);
        out.push_back(kTbl[(v >> 6) & 63]);
        out.push_back(kTbl[v & 63]);
        i += 3;
    }
    const size_t rem = in.size() - i;
    if (rem == 1) {
        const unsigned v = static_cast<unsigned char>(in[i]) << 16;
        out.push_back(kTbl[(v >> 18) & 63]);
        out.push_back(kTbl[(v >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (rem == 2) {
        const unsigned v = (static_cast<unsigned char>(in[i]) << 16) |
                           (static_cast<unsigned char>(in[i + 1]) << 8);
        out.push_back(kTbl[(v >> 18) & 63]);
        out.push_back(kTbl[(v >> 12) & 63]);
        out.push_back(kTbl[(v >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

// 图像文件 → data URL。QwenAPI 的 image_url 约定由调用方拼好完整 `data:...;base64,`
// 前缀（QwenAPI.cpp 里直接塞进 {"image_url":{"url":...}}，不再二次处理）。
std::string imageToDataUrl(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string bytes = ss.str();
    if (bytes.empty()) return "";
    std::string mime = "image/jpeg";
    const std::string ext = std::filesystem::path(path).extension().string();
    if (ext == ".png")       mime = "image/png";
    else if (ext == ".webp") mime = "image/webp";
    else if (ext == ".gif")  mime = "image/gif";
    else if (ext == ".bmp")  mime = "image/bmp";
    return "data:" + mime + ";base64," + base64Encode(bytes);
}

// Q9 二次确认用：把串里所有 ASCII 字母/数字**按原顺序压紧**（大写归一，丢掉分隔符、
// 标点与中文叙述）。
//
// 🔴 压紧而非分词是刻意的——分词对空格敏感会误判：实测 sign_level2a 的 caption 读
// 「2A」（连续 ⇒ 一个 token），而定向复读读「2 A」（空格 ⇒ 两个 token），集合比对
// 判「不一致」而把一条**读对了的**事实错误降级。压紧后两者都是 "2A"，子串比对即正确
// 匹配；而漂移案例（caption「PARENTA」vs 复读「看不清」→ 压紧后 "" ）仍判不一致。
std::string asciiCompact(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
            out.push_back(static_cast<char>(c));
        } else if (c >= 'a' && c <= 'z') {
            out.push_back(static_cast<char>(std::toupper(c)));
        }
    }
    return out;
}
}  // namespace

CockpitAssistant::CockpitAssistant(MemorySystem& mem, QwenAPI& llm, QwenAPI* vlm)
    : mem_(mem), llm_(llm), vlm_(vlm) {}

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
    own.strict = true;   // fail-closed：多驾驶员场景开 strict，driver_ 异常为空时也不泄露全部
    GovernanceFilter shared;
    shared.scope = std::string("shared");
    shared.strict = true;

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

// ---------------------------------------------------------------------------
// Q9 多模态轮次：图像 → VLM caption → extract → ingest(authority=observed)
// ---------------------------------------------------------------------------
// 与 respond() 的分工：respond 处理驾驶员口述（authority=asserted），observe 处理
// 摄像头帧（authority=observed）。两者共用同一记忆核，authority 闸门在 ingest 里
// 自动裁决冲突——这是 Q9「让 authority 从只做过滤升级为参与消解」的应用层落点。
//
// ⚠️ observe 的候选 authority 恒为 observed（最高），所以闸门只可能「放行」或
// 「把 UPDATE 升级为 SUPERSEDE」，**永远不会拦下**。拦下只发生在 respond（asserted
// 撞上已有 observed）——那正是 demo 要展示的非对称行为。
// Q9：向 VLM 发一次 (prompt + 图像) 请求。temperature=0 保证可复现。
std::string CockpitAssistant::vlmAsk(const std::string& prompt,
                                     const std::string& imageDataUrl) {
    QwenChatCompletionRequest req;
    req.model = kVlmModel;
    req.temperature = 0.0f;
    req.top_p = 1.0f;
    req.max_tokens = 256;
    QwenMessage msg;
    msg.role = "user";
    QwenContent tc; tc.type = "text";      tc.text      = prompt;
    QwenContent ic; ic.type = "image_url"; ic.image_url = imageDataUrl;
    msg.contents.push_back(tc);
    msg.contents.push_back(ic);
    req.messages.push_back(msg);
    return vlm_->chat_completion(req).content;
}

CockpitAssistant::ObserveResult CockpitAssistant::observe(const std::string& imagePath) {
    ObserveResult r;
    r.driver = driver_;
    r.imagePath = imagePath;

    if (!vlm_) {
        std::cerr << "[CockpitAssistant] VLM 未配置（config.ini [api] vlm_url 为空）"
                     "——多模态轮次跳过\n";
        return r;
    }

    // ① 图像 → data URL（QwenAPI 约定由调用方拼好完整 data: 前缀）
    const std::string dataUrl = imageToDataUrl(imagePath);
    if (dataUrl.empty()) {
        std::cerr << "[CockpitAssistant] 读不到图像或文件为空: " << imagePath << "\n";
        return r;
    }

    // ② VLM caption（V4 持久状态式）。
    //    🔴 分辨率由**服务端** `--image-max-tokens 300` 钉住，C++ 侧不缩放、不引图像库。
    //    实测该配置等价客户端 le=640（M4b 6/7、prompt_tok 均值 422 vs 402、~1.5s/帧），
    //    而 400 档只有 5/7、1600 档只 3/7 ⇒ 300 是实测最优档，必须写进部署口径。
    r.caption = vlmAsk(kCaptionPrompt, dataUrl);
    if (r.caption.empty() || r.caption.rfind("Error:", 0) == 0) {
        std::cerr << "[CockpitAssistant] VLM caption 失败: " << r.caption << "\n";
        r.caption.clear();
        return r;
    }

    // ③ 提取——文本侧仍走 qwen3-4b（经 MemorySystem 门面）。VLM 只做 caption，
    //    不顶替文本模型，否则 Q2/Q4 的标定全部失效（方案 §1.2）。
    r.extracted = mem_.extract(r.caption, "");

    // ④ Q9 二次确认（E4b 缓解）：caption 里若含 ASCII 文字/数字字段，对**同一帧**再做
    //    一次中性定向复读；复读确认不了的字段 → 该事实 authority 降为 inferred（最低
    //    可信度，不覆盖任何记忆、反可被任何来源覆盖）。
    //    实测能挡住「读数随分辨率漂移」：座椅标签叙述式硬读「PARENTA」、定向复读一致
    //    答「看不清」⇒ 判不可信。⚠️ 挡不住「稳定误读」（speed20 两种 prompt 都读 2.0）。
    //    比对用 asciiCompact（压紧子串匹配）而非分词集合——分词对空格敏感会误判
    //    （caption「2A」vs 复读「2 A」，见 asciiCompact 注释）。
    std::string confirmed;   // 复读结果的压紧 ASCII
    bool needVerify = false;
    for (const auto& f : r.extracted)
        if (!asciiCompact(f.text).empty()) { needVerify = true; break; }
    if (needVerify) {
        r.verification = vlmAsk(kVerifyPrompt, dataUrl);
        confirmed = asciiCompact(r.verification);
    }

    // ⑤ 消解 + 落库：authority=observed（文字字段未获复读确认则降为 inferred）、
    //    source_image=帧路径
    for (const auto& f : r.extracted) {
        std::string scope = f.scope;
        if (scope != "private" && scope != "shared") scope = "private";

        std::string authority = "observed";
        const std::string fc = asciiCompact(f.text);
        if (!fc.empty() && confirmed.find(fc) == std::string::npos) {
            authority = "inferred";
            ++r.downgraded;
        }
        r.factAuthority.push_back(authority);

        const IngestResult ir = mem_.ingest(f.text, f.type, driver_, scope,
                                            authority, "cockpit-cam", 0, imagePath);
        r.ingested.push_back(ir);
        if (ir.action == MemoryAction::Superseded && r.supersedeChain.empty())
            r.supersedeChain = mem_.history(ir.id);
    }
    return r;
}
