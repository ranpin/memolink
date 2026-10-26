// ============================================================================
// 座舱助手 CLI —— memolink app 层第二个公民（第一个是 memolink CLI）
// ----------------------------------------------------------------------------
//   cockpit_assistant seed [--corpus path] [--owner 张三] [--db path]   载入语料
//   cockpit_assistant run  <script.json> [--db path]                    跑脚本化对话
//   cockpit_assistant chat [--owner 张三] [--db path]                   交互式
//
// 只依赖接口层门面 MemorySystem + QwenAPI，core/infra 一行不动。run 模式输出带
// 机制标注的真实转录（检索意图/治理拦截/五操作/版本链），供 demo 页回放。
// ============================================================================
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <json/json.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "infra/storage/ConfigManager.h"
#include "memory/api/MemorySystem.h"
#include "infra/llm/QwenAPI.h"
#include "app/cockpit/CockpitAssistant.h"

static void usage() {
    std::cerr <<
        "座舱助手（memolink app 层第二公民）\n"
        "用法:\n"
        "  cockpit_assistant seed    [--corpus path] [--owner 张三] [--db path]\n"
        "  cockpit_assistant run     <script.json> [--db path] [--dump store.json]\n"
        "  cockpit_assistant chat    [--owner 张三] [--db path]\n"
        "  cockpit_assistant observe <image.jpg> [--owner 张三] [--db path]\n"
        "      （Q9 多模态轮次：图像 → VLM caption → observed 记忆；需 config.ini [api] vlm_url）\n";
}

static std::string getOpt(const std::vector<std::string>& args, const std::string& key,
                          const std::string& def = "") {
    for (size_t i = 0; i + 1 < args.size(); ++i)
        if (args[i] == key) return args[i + 1];
    return def;
}

// 第一个非选项位置参数（跳过 --db 这类带值选项）
static std::string getPositional(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--db" || args[i] == "--corpus" || args[i] == "--owner" ||
            args[i] == "--dump") { ++i; continue; }
        if (args[i].rfind("--", 0) == 0) continue;
        return args[i];
    }
    return "";
}

// 把一轮对话的真实运作打印成带机制标注的转录（demo 回放的素材）
static void printTurn(const CockpitAssistant::TurnResult& r, int turnNo) {
    std::cout << "\n════════════════ 第 " << turnNo << " 轮 ════════════════\n";
    std::cout << "[驾驶员] " << r.driver << "\n";
    std::cout << "[用户] " << r.userText << "\n";

    std::cout << "[检索] 类型意图=" << r.intent
              << "，治理=(owner=" << r.driver << " ∪ scope=shared)\n";
    if (r.recalled.empty()) {
        std::cout << "  （无可见命中）\n";
    } else {
        for (const auto& m : r.recalled)
            std::cout << "  #" << std::setw(2) << m.id
                      << " [" << memoryTypeName(m.type) << "/" << m.scope << "/" << m.owner << "]"
                      << " score=" << std::fixed << std::setprecision(3) << m.score
                      << "  " << m.content << "\n";
    }

    if (!r.blocked.empty()) {
        std::cout << "[治理拦截] 无过滤本会命中、但对 " << r.driver << " 不可见的私人记忆:\n";
        for (const auto& m : r.blocked)
            std::cout << "  #" << std::setw(2) << m.id
                      << " [" << memoryTypeName(m.type) << "/" << m.scope << "/" << m.owner << "]"
                      << "  " << m.content << "\n";
    }

    std::cout << "[回复] " << r.response << "\n";

    if (r.extracted.empty()) {
        std::cout << "[写入] 无可记事实（NOOP）\n";
    } else {
        std::cout << "[写入] 提取 " << r.extracted.size() << " 条候选事实:\n";
        for (size_t i = 0; i < r.extracted.size(); ++i) {
            const auto& f = r.extracted[i];
            std::cout << "  [" << memoryTypeName(f.type) << "] 「" << f.text << "」 → ";
            if (i < r.ingested.size()) {
                const auto& ir = r.ingested[i];
                std::cout << memoryActionName(ir.action) << " id=" << ir.id
                          << " sim=" << std::fixed << std::setprecision(3) << ir.similarity
                          << (ir.usedLlm ? " [System-2]" : " [System-1]")
                          << (ir.blockedByAuthority ? " [authority拦下]" : "")
                          << (ir.authorityPromoted ? " [authority升级]" : "") << "\n";
                if (ir.blockedByAuthority)
                    std::cout << "      ↳ 口述(asserted) 撞上更高可信度的观察(observed)，"
                                 "不静默覆盖 → 两条并存\n";
            } else {
                std::cout << "?\n";
            }
        }
    }

    if (!r.supersedeChain.empty()) {
        std::cout << "[版本链] SUPERSEDE 演化（旧→新）:\n";
        for (const auto& m : r.supersedeChain)
            std::cout << "  #" << std::setw(2) << m.id << (m.valid ? " [有效]" : " [失效]")
                      << "  " << m.content << "\n";
    }
}

// Q9 多模态轮次的转录：图像 → caption → 二次确认 → observed/inferred 记忆
static void printObserve(const CockpitAssistant::ObserveResult& r, int turnNo) {
    std::cout << "\n════════════════ 第 " << turnNo << " 轮 [摄像头] ════════════════\n";
    std::cout << "[驾驶员] " << r.driver << "\n";
    std::cout << "[图像] " << r.imagePath << "\n";
    if (r.caption.empty()) {
        std::cout << "[caption] （VLM 未配置或图像读取失败，本轮无写入）\n";
        return;
    }
    std::cout << "[caption] " << r.caption << "\n";

    // Q9 二次确认（E4b 缓解）：对同一帧做中性定向复读，文字字段对不上就降为 inferred
    if (!r.verification.empty()) {
        std::cout << "[二次确认] 定向复读: " << r.verification << "\n";
        if (r.downgraded > 0)
            std::cout << "           ↳ " << r.downgraded
                      << " 条事实的文字字段未获复读确认 → authority 降为 inferred"
                         "（最低可信度，不覆盖任何记忆）\n";
    }

    if (r.extracted.empty()) {
        std::cout << "[写入] 无可记事实（NOOP）\n";
    } else {
        std::cout << "[写入] 提取 " << r.extracted.size()
                  << " 条候选事实（来源=cockpit-cam）:\n";
        for (size_t i = 0; i < r.extracted.size(); ++i) {
            const auto& f = r.extracted[i];
            const std::string auth =
                i < r.factAuthority.size() ? r.factAuthority[i] : std::string("?");
            std::cout << "  [" << memoryTypeName(f.type) << "/" << auth << "] 「"
                      << f.text << "」 → ";
            if (i < r.ingested.size()) {
                const auto& ir = r.ingested[i];
                std::cout << memoryActionName(ir.action) << " id=" << ir.id
                          << " sim=" << std::fixed << std::setprecision(3) << ir.similarity
                          << (ir.usedLlm ? " [System-2]" : " [System-1]")
                          << (ir.authorityPromoted ? " [authority升级]" : "")
                          << (auth == "inferred" ? " [文字未确认→降级]" : "") << "\n";
                if (ir.authorityPromoted)
                    std::cout << "      ↳ 观察(observed) 可信度高于口述(asserted)，"
                                 "UPDATE 升级为 SUPERSEDE 以保留版本链\n";
            } else {
                std::cout << "?\n";
            }
        }
    }

    if (!r.supersedeChain.empty()) {
        std::cout << "[版本链] SUPERSEDE 演化（旧→新，标注 authority）:\n";
        for (const auto& m : r.supersedeChain)
            std::cout << "  #" << std::setw(2) << m.id << (m.valid ? " [有效]" : " [失效]")
                      << " [" << m.authority << "]  " << m.content << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    const std::string cmd = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);

    // 座舱助手用独立 DB，不污染 memolink CLI 的库
    const std::string dbPath = getOpt(args, "--db", "../database/cockpit.db");
    if (!std::filesystem::exists(dbPath)) {
        if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    QwenAPI llm;   // 走 config.ini 的 local_url（本地 llama-server，文本 LLM :8099）

    // Q9：VLM 实例（:8098）。config.ini [api] vlm_url 非空才启用多模态轮次。
    // 用实例级 override 让同一进程同时调两个 llama-server——QwenAPI 的本地 URL
    // 原本只从 config 的 api.local_url 读，set_base_url 只影响 DashScope 分支。
    QwenAPI vlm;
    QwenAPI* vlmPtr = nullptr;
    const std::string vlmUrl = ConfigManager::getInstance().getVlmApiUrl();
    if (!vlmUrl.empty()) {
        vlm.set_local_url_override(vlmUrl);
        vlmPtr = &vlm;
    }

    MemorySystem mem(storage, &llm);
    if (!mem.ready()) { std::cerr << "store 未就绪（config.ini [edge] 没配模型？）\n"; return 1; }
    CockpitAssistant assistant(mem, llm, vlmPtr);
    if (!assistant.vlmReady())
        std::cerr << "[提示] config.ini [api] vlm_url 为空——observe 多模态轮次不可用，"
                     "文本轮照常\n";

    if (cmd == "seed") {
        const std::string corpus = getOpt(args, "--corpus", "../bench/corpus/cockpit.json");
        const std::string owner = getOpt(args, "--owner", "张三");
        const int n = assistant.seedFromCorpus(corpus, owner);
        const auto st = mem.stats();
        std::cout << "已载入 " << n << " 条记忆（owner=" << owner << "）\n"
                  << "当前库: total=" << st.total << " semantic=" << st.semantic
                  << " episodic=" << st.episodic << " procedural=" << st.procedural << "\n";
        return 0;
    }

    if (cmd == "run") {
        const std::string scriptPath = getPositional(args);
        if (scriptPath.empty()) { usage(); return 1; }
        const std::string dumpPath = getOpt(args, "--dump", "");
        std::ifstream ifs(scriptPath);
        if (!ifs.is_open()) { std::cerr << "打不开脚本: " << scriptPath << "\n"; return 1; }
        std::stringstream ss; ss << ifs.rdbuf();
        Json::Value script;
        Json::CharReaderBuilder rb; std::string errs;
        std::istringstream is(ss.str());
        if (!Json::parseFromStream(rb, is, &script, &errs)) {
            std::cerr << "脚本 JSON 解析失败: " << errs << "\n"; return 1;
        }

        // --dump：seed 后与每轮后各 dump 一次全库快照（含失效记忆），供 demo 页
        // 「记忆库」面板回放每轮数据库状态变化。假设全新库（跑前 rm cockpit.db）。
        Json::Value snapshots(Json::arrayValue);
        auto dumpSnapshot = [&](const std::string& label) {
            if (dumpPath.empty()) return;
            std::vector<Memory> all = mem.list(std::nullopt, 0, /*includeInvalid=*/true);
            std::sort(all.begin(), all.end(),
                      [](const Memory& a, const Memory& b) { return a.id < b.id; });
            Json::Value mems(Json::arrayValue);
            for (const auto& m : all) {
                Json::Value jm;
                jm["id"] = m.id;
                jm["type"] = memoryTypeName(m.type);
                jm["content"] = m.content;
                jm["scope"] = m.scope;
                jm["owner"] = m.owner;
                jm["authority"] = m.authority;       // Q9：demo 页区分 observed/asserted
                jm["sourceImage"] = m.sourceImage;   // Q9：摄像头来源帧路径
                jm["valid"] = m.valid;
                jm["supersededBy"] = m.supersededBy;
                jm["supersedes"] = m.supersedes;
                mems.append(jm);
            }
            Json::Value snap;
            snap["label"] = label;
            snap["memories"] = mems;
            snapshots.append(snap);
        };

        // 库为空且脚本带 seed 配置 → 自动载入语料
        if (mem.stats().total == 0 && script.isMember("seed")) {
            const std::string corpus =
                script["seed"].get("corpus", "../bench/corpus/cockpit.json").asString();
            const std::string owner = script["seed"].get("owner", "张三").asString();
            const int n = assistant.seedFromCorpus(corpus, owner);
            std::cout << "[seed] 库为空，已从 " << corpus << " 载入 " << n
                      << " 条记忆（owner=" << owner << "）\n";
        }
        dumpSnapshot("seed");

        const Json::Value& turns = script["turns"];
        int turnNo = 0;
        for (const auto& t : turns) {
            assistant.setDriver(t.get("driver", "张三").asString());
            // Q9 多模态轮次：脚本轮带 "image" 字段 → 摄像头帧 → VLM caption → observed 记忆
            const std::string image = t.get("image", "").asString();
            if (!image.empty()) {
                printObserve(assistant.observe(image), ++turnNo);
                dumpSnapshot("turn" + std::to_string(turnNo));
                continue;
            }
            const std::string text = t.get("text", "").asString();
            if (text.empty()) continue;
            const int topK = t.get("topk", 4).asInt();
            printTurn(assistant.respond(text, topK), ++turnNo);
            dumpSnapshot("turn" + std::to_string(turnNo));
        }

        if (!dumpPath.empty()) {
            Json::Value root;
            root["snapshots"] = snapshots;
            Json::StreamWriterBuilder wb;
            wb["indentation"] = "";
            std::ofstream ofs(dumpPath);
            ofs << Json::writeString(wb, root);
            std::cout << "[dump] " << snapshots.size() << " 个快照 → " << dumpPath << "\n";
        }

        const auto st = mem.stats();
        std::cout << "\n════════════════ 结束 ════════════════\n"
                  << "最终库: total=" << st.total << " valid=" << st.valid
                  << " invalid=" << st.invalid << "\n";
        return 0;
    }

    if (cmd == "chat") {
        assistant.setDriver(getOpt(args, "--owner", "张三"));
        std::cout << "座舱助手（驾驶员=" << assistant.driver() << "），输入问题，Ctrl-D 退出。\n";
        std::string line;
        int turnNo = 0;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            printTurn(assistant.respond(line, 4), ++turnNo);
            std::cout << "> " << std::flush;
        }
        return 0;
    }

    // Q9 多模态轮次：单张图像 → VLM caption → observed 记忆
    if (cmd == "observe") {
        const std::string image = getPositional(args);
        if (image.empty()) { usage(); return 1; }
        assistant.setDriver(getOpt(args, "--owner", "张三"));
        printObserve(assistant.observe(image), 1);
        const auto st = mem.stats();
        std::cout << "\n当前库: total=" << st.total << " valid=" << st.valid
                  << " invalid=" << st.invalid << "\n";
        return 0;
    }

    usage();
    return 1;
}
