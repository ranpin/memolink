// ============================================================================
// Q6：mirix-mem CLI——八命令串起完整记忆管线（docs/memory_v2_design.md §11）
// ----------------------------------------------------------------------------
//   mirix-mem extract  --user "..." --assistant "..."   # ① 提取生成（MemoryExtractor）
//   mirix-mem ingest   "..." --type semantic            # ②③ 消解+存储（MemoryResolver）
//   mirix-mem recall   "..." --top 4 [--type episodic] [--history]  # ④ 检索
//   mirix-mem update   <id> "..."                       # 显式更新
//   mirix-mem remove   <id>                             # 记录级删除
//   mirix-mem list     [--type semantic] [--invalid]    # 列举
//   mirix-mem history  <id>                             # 版本链回溯
//   mirix-mem stats                                     # 统计
//
// 这是 demo 能被看见的前提（§11）：demo 页的「生命周期演示」就是回放这些命令的真实输出。
// 应用层只依赖接口层门面 MemorySystem，不直接碰 core 三件套（Store/Resolver/Extractor）。
// ============================================================================

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/MemorySystem.h"
#include "models/QwenAPI.h"

static void usage() {
    std::cerr <<
        "用法:\n"
        "  mirix-mem extract  --user \"...\" --assistant \"...\"   提取候选事实\n"
        "  mirix-mem ingest   \"...\" --type semantic|episodic|procedural  消解+存储\n"
        "  mirix-mem recall   \"...\" [--top N] [--type T] [--history]  检索\n"
        "  mirix-mem update   <id> \"...\"    更新\n"
        "  mirix-mem remove   <id>           删除\n"
        "  mirix-mem list     [--type T] [--invalid]  列举\n"
        "  mirix-mem history  <id>           版本链\n"
        "  mirix-mem stats                   统计\n";
}

static const char* actionName(MemoryAction a) { return memoryActionName(a); }

static void printMemory(const Memory& m, bool withScore = false) {
    std::cout << "  #" << std::setw(3) << m.id
              << " [" << memoryTypeName(m.type) << "]"
              << (m.valid ? "" : " [失效]");
    if (withScore) std::cout << " score=" << std::fixed << std::setprecision(3) << m.score;
    std::cout << "  " << m.content << "\n";
}

// 从 args 里取 --key value；没有返回 def
static std::string getOpt(const std::vector<std::string>& args, const std::string& key,
                          const std::string& def = "") {
    for (size_t i = 0; i + 1 < args.size(); ++i)
        if (args[i] == key) return args[i + 1];
    return def;
}
static bool hasFlag(const std::vector<std::string>& args, const std::string& key) {
    for (const auto& a : args) if (a == key) return true;
    return false;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    const std::string cmd = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);

    const std::string dbPath = "../database/mirix_mem.db";
    if (!std::filesystem::exists(dbPath)) {
        if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    QwenAPI llm;   // 走 config.ini 的 local_url（本地 llama-server）
    // 接口层门面：内部按 store_ → resolver_ → extractor_ 顺序组装 core 三件套
    MemorySystem mem(storage, &llm);
    if (!mem.ready()) { std::cerr << "store 未就绪（config.ini [edge] 没配模型？）\n"; return 1; }

    if (cmd == "extract") {
        const std::string user = getOpt(args, "--user");
        const std::string assistant = getOpt(args, "--assistant");
        auto facts = mem.extract(user, assistant);
        std::cout << "提取出 " << facts.size() << " 条候选事实:\n";
        for (const auto& f : facts)
            std::cout << "  [" << memoryTypeName(f.type) << "/" << f.owner << "/" << f.scope
                      << "] " << f.text << "\n";
        if (facts.empty()) std::cout << "  （无可记事实 → NOOP）\n";
        return 0;
    }

    if (cmd == "ingest") {
        if (args.empty()) { usage(); return 1; }
        const std::string text = args[0];
        MemoryType type = MemoryType::Semantic;
        memoryTypeFromName(getOpt(args, "--type", "semantic"), type);
        IngestResult r = mem.ingest(text, type);
        std::cout << "动作=" << actionName(r.action)
                  << " id=" << r.id
                  << " sim=" << std::fixed << std::setprecision(3) << r.similarity
                  << (r.usedLlm ? " [System-2]" : " [System-1]") << "\n"
                  << "  理由: " << r.reason << "\n";
        return 0;
    }

    if (cmd == "recall") {
        if (args.empty()) { usage(); return 1; }
        const std::string query = args[0];
        const int top = std::stoi(getOpt(args, "--top", "4"));
        std::optional<MemoryType> filter;
        const std::string tname = getOpt(args, "--type");
        if (!tname.empty()) {
            MemoryType t;
            if (memoryTypeFromName(tname, t)) filter = t;
        }
        const bool history = hasFlag(args, "--history");
        auto hits = mem.recall(query, top, filter, history);
        std::cout << "查询「" << query << "」 top-" << top << ":\n";
        for (const auto& m : hits) printMemory(m, /*withScore=*/true);
        if (hits.empty()) std::cout << "  （无命中）\n";
        return 0;
    }

    if (cmd == "update") {
        if (args.size() < 2) { usage(); return 1; }
        const int id = std::stoi(args[0]);
        const std::string text = args[1];
        const bool ok = mem.update(id, text);
        std::cout << (ok ? "已更新 #" : "更新失败 #") << id << "\n";
        return ok ? 0 : 1;
    }

    if (cmd == "remove") {
        if (args.empty()) { usage(); return 1; }
        const int id = std::stoi(args[0]);
        const bool ok = mem.remove(id);
        std::cout << (ok ? "已删除 #" : "删除失败 #") << id << "\n";
        return ok ? 0 : 1;
    }

    if (cmd == "list") {
        std::optional<MemoryType> filter;
        const std::string tname = getOpt(args, "--type");
        if (!tname.empty()) {
            MemoryType t;
            if (memoryTypeFromName(tname, t)) filter = t;
        }
        const bool invalid = hasFlag(args, "--invalid");
        auto rows = mem.list(filter, 0, invalid);
        std::cout << "共 " << rows.size() << " 条" << (invalid ? "（含失效）" : "") << ":\n";
        for (const auto& m : rows) printMemory(m);
        return 0;
    }

    if (cmd == "history") {
        if (args.empty()) { usage(); return 1; }
        const int id = std::stoi(args[0]);
        auto chain = mem.history(id);
        std::cout << "#" << id << " 的版本链（旧→新）:\n";
        for (const auto& m : chain) printMemory(m);
        if (chain.empty()) std::cout << "  （无此记忆或无版本链）\n";
        return 0;
    }

    if (cmd == "stats") {
        auto st = mem.stats();
        std::cout << "total=" << st.total << " valid=" << st.valid << " invalid=" << st.invalid
                  << "\n  semantic=" << st.semantic << " episodic=" << st.episodic
                  << " procedural=" << st.procedural << "\n";
        return 0;
    }

    usage();
    return 1;
}
