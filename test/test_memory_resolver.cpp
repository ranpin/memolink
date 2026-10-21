#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <sqlite3.h>
#include <json/json.h>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/EdgeMemoryStore.h"
#include "database/managers/MemoryResolver.h"
#include "models/QwenAPI.h"

// ============================================================================
// Q2 验证：MemoryResolver 五操作消解。
// 对 bench/eval/resolve_scenarios.json 的每个标注场景：建库 → 灌 seed →
// 消解 candidate → 比对动作与目标 id。输出五分类准确率 + System-1 覆盖率 +
// 混淆明细。
//
// 用法: test_memory_resolver [scenarios.json] [db路径]
// 前置：config.ini 的 [edge] 段配好模型/分词器；[api] local_url 指向本地
//       llama-server（System-2 用），否则模糊地带会回退 CREATE。
// ============================================================================

static MemoryAction actionFromExpected(const std::string& s) {
    if (s == "CREATE")    return MemoryAction::Created;
    if (s == "UPDATE")    return MemoryAction::Updated;
    if (s == "REINFORCE") return MemoryAction::Reinforced;
    if (s == "SUPERSEDE") return MemoryAction::Superseded;
    if (s == "NOOP")      return MemoryAction::Noop;
    return MemoryAction::Failed;
}

static void clearMemory(StorageManager& storage) {
    sqlite3* db = storage.getDatabase();
    if (db) sqlite3_exec(db, "DELETE FROM memory;", nullptr, nullptr, nullptr);
}

int main(int argc, char** argv) {
    const std::string scenPath = argc > 1 ? argv[1] : "../bench/eval/resolve_scenarios.json";
    const std::string dbPath   = argc > 2 ? argv[2] : "../database/resolver_test.db";

    // ---- 读场景集 ----
    Json::Value root;
    {
        std::ifstream f(scenPath);
        if (!f.is_open()) { std::cerr << "打不开场景集: " << scenPath << "\n"; return 2; }
        Json::CharReaderBuilder rb;
        std::string errs;
        if (!Json::parseFromStream(rb, f, &root, &errs)) {
            std::cerr << "场景集解析失败: " << errs << "\n"; return 2;
        }
    }
    const Json::Value& scenarios = root["scenarios"];
    if (!scenarios.isArray() || scenarios.empty()) {
        std::cerr << "场景集为空\n"; return 2;
    }

    // ---- 建库 ----
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    EdgeMemoryStore store(storage);
    if (!store.ready()) { std::cerr << "store 未就绪（[edge] 模型路径没配？）\n"; return 1; }

    QwenAPI llm;                       // 走 config.ini 的 local_url（本地 llama-server）
    MemoryResolver resolver(store, &llm);

    std::cout << "=== Q2 MemoryResolver 五操作消解评测 ===\n"
              << "  场景数=" << scenarios.size()
              << "  embed=" << store.embedModelTag()
              << "  θ_new=" << MemoryResolver::kThetaNew
              << "  θ_dup=" << MemoryResolver::kThetaDup << "\n\n";

    // ---- 统计容器 ----
    const std::vector<std::string> classes = {"CREATE", "UPDATE", "REINFORCE", "SUPERSEDE", "NOOP"};
    std::map<std::string, int> total, correct, usedLlmCnt;
    std::map<std::string, std::map<std::string, int>> confusion;  // expected → predicted → n
    int overall = 0, overallOk = 0, llmCalls = 0, targetTot = 0, targetOk = 0;

    for (const auto& s : scenarios) {
        const std::string id = s["id"].asString();
        const std::string expected = s["expected"].asString();
        const int expectedTarget = s.get("expected_target", -1).asInt();
        MemoryType type = MemoryType::Semantic;
        memoryTypeFromName(s.get("type", "semantic").asString(), type);

        clearMemory(storage);

        // 灌 seed（直接 CREATE），记录 id 顺序
        std::vector<int> seedIds;
        for (const auto& seed : s["seed"]) {
            auto r = store.ingestText(seed.asString(), type);
            seedIds.push_back(r.id);
        }

        // 消解 candidate
        const ResolveDecision d = resolver.resolve(s["candidate"].asString(), type);
        const std::string predicted = memoryActionName(d.action);
        // memoryActionName 返回 CREATED/UPDATED/... 去掉尾 D 对齐 expected 命名
        std::string predLabel = predicted;
        if (predLabel == "CREATED") predLabel = "CREATE";
        else if (predLabel == "UPDATED") predLabel = "UPDATE";
        else if (predLabel == "REINFORCED") predLabel = "REINFORCE";
        else if (predLabel == "SUPERSEDED") predLabel = "SUPERSEDE";

        const bool actionOk = (d.action == actionFromExpected(expected));
        bool tgtOk = true;
        if (expectedTarget >= 0 && actionOk) {
            tgtOk = (expectedTarget < (int)seedIds.size()) && (d.targetId == seedIds[expectedTarget]);
        }

        total[expected]++; overall++;
        confusion[expected][predLabel]++;
        if (d.usedLlm) { usedLlmCnt[expected]++; llmCalls++; }
        if (actionOk) { correct[expected]++; overallOk++; }
        if (expectedTarget >= 0 && actionOk) { targetTot++; if (tgtOk) targetOk++; }

        const char* mark = actionOk ? (tgtOk ? "✓" : "△目标错") : "✗";
        std::cout << "  " << mark << " " << id
                  << "  期望=" << expected << " 实得=" << predLabel
                  << "  sim=" << d.similarity
                  << (d.usedLlm ? " [S2]" : " [S1]");
        if (expectedTarget >= 0 && actionOk)
            std::cout << "  tgt=" << d.targetId << "/" << seedIds[expectedTarget];
        if (!actionOk) std::cout << "\n      reason: " << d.reason;
        std::cout << "\n";
    }

    // ---- 汇总 ----
    std::cout << "\n=== 五分类准确率 ===\n";
    std::cout << "  类别        正确/总数   准确率   走S2数\n";
    for (const auto& c : classes) {
        if (!total[c]) continue;
        const double acc = 100.0 * correct[c] / total[c];
        std::cout << "  " << c;
        for (size_t i = c.size(); i < 11; ++i) std::cout << " ";
        std::cout << " " << correct[c] << "/" << total[c]
                  << "      " << acc << "%"
                  << "    " << usedLlmCnt[c] << "\n";
    }
    const double overallAcc = overall ? 100.0 * overallOk / overall : 0.0;
    std::cout << "\n  总体: " << overallOk << "/" << overall << " = " << overallAcc << "%\n";

    // System-1 覆盖率：完全没调 LLM 的比例
    const int s1 = overall - llmCalls;
    std::cout << "  System-1 覆盖率（未调 LLM）: " << s1 << "/" << overall
              << " = " << (overall ? 100.0 * s1 / overall : 0.0) << "%\n";
    if (targetTot)
        std::cout << "  目标定位正确率（动作对的前提下）: " << targetOk << "/" << targetTot << "\n";

    // ---- 混淆矩阵 ----
    std::cout << "\n=== 混淆矩阵（行=期望，列=实得）===\n";
    std::cout << "  期望\\实得    ";
    for (const auto& c : classes) std::cout << c.substr(0, 5) << " ";
    std::cout << "\n";
    for (const auto& e : classes) {
        if (!total[e]) continue;
        std::cout << "  " << e;
        for (size_t i = e.size(); i < 12; ++i) std::cout << " ";
        for (const auto& p : classes) {
            const int n = confusion[e][p];
            std::cout << (n ? std::to_string(n) : ".") << "     ";
        }
        std::cout << "\n";
    }

    std::cout << "\n=== 结果: " << overallOk << "/" << overall << " 正确 ===\n";
    return 0;   // 评测工具：即使有错也返回 0，靠打印的准确率判断
}
