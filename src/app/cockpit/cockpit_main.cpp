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
#include <json/json.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "memory/api/MemorySystem.h"
#include "infra/llm/QwenAPI.h"
#include "app/cockpit/CockpitAssistant.h"

static void usage() {
    std::cerr <<
        "座舱助手（memolink app 层第二公民）\n"
        "用法:\n"
        "  cockpit_assistant seed [--corpus path] [--owner 张三] [--db path]\n"
        "  cockpit_assistant run  <script.json> [--db path]\n"
        "  cockpit_assistant chat [--owner 张三] [--db path]\n";
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
        if (args[i] == "--db" || args[i] == "--corpus" || args[i] == "--owner") { ++i; continue; }
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
                          << (ir.usedLlm ? " [System-2]" : " [System-1]") << "\n";
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

    QwenAPI llm;   // 走 config.ini 的 local_url（本地 llama-server）
    MemorySystem mem(storage, &llm);
    if (!mem.ready()) { std::cerr << "store 未就绪（config.ini [edge] 没配模型？）\n"; return 1; }
    CockpitAssistant assistant(mem, llm);

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
        std::ifstream ifs(scriptPath);
        if (!ifs.is_open()) { std::cerr << "打不开脚本: " << scriptPath << "\n"; return 1; }
        std::stringstream ss; ss << ifs.rdbuf();
        Json::Value script;
        Json::CharReaderBuilder rb; std::string errs;
        std::istringstream is(ss.str());
        if (!Json::parseFromStream(rb, is, &script, &errs)) {
            std::cerr << "脚本 JSON 解析失败: " << errs << "\n"; return 1;
        }

        // 库为空且脚本带 seed 配置 → 自动载入语料
        if (mem.stats().total == 0 && script.isMember("seed")) {
            const std::string corpus =
                script["seed"].get("corpus", "../bench/corpus/cockpit.json").asString();
            const std::string owner = script["seed"].get("owner", "张三").asString();
            const int n = assistant.seedFromCorpus(corpus, owner);
            std::cout << "[seed] 库为空，已从 " << corpus << " 载入 " << n
                      << " 条记忆（owner=" << owner << "）\n";
        }

        const Json::Value& turns = script["turns"];
        int turnNo = 0;
        for (const auto& t : turns) {
            const std::string text = t.get("text", "").asString();
            if (text.empty()) continue;
            assistant.setDriver(t.get("driver", "张三").asString());
            const int topK = t.get("topk", 4).asInt();
            printTurn(assistant.respond(text, topK), ++turnNo);
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

    usage();
    return 1;
}
