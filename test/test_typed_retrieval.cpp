#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <iomanip>
#include <json/json.h>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/EdgeMemoryStore.h"

// ============================================================================
// Q3 验证：类型特化检索 vs 单一策略基线，比 hit@1（Recall@1）。
// ----------------------------------------------------------------------------
// 基线   = recall()：纯余弦、全类型一起搜、不加权（v1 的单一策略）。
// 类型特化 = recallTyped()：查询意图识别 → 同类型加权 ×kTypeBoost + episodic 时间衰减。
// 闸门（docs/memory_v2_design.md §7）：类型特化的 Recall@1 是否高于基线。
// 同时单独报意图识别准确率——它是类型路由的前提，错了加权就会帮倒忙。
//
// 用法: test_typed_retrieval [benchmark.json] [db路径]
// 前置：config.ini 的 [edge] 段配好模型/分词器路径。
// ============================================================================

int main(int argc, char** argv) {
    const std::string benchPath = argc > 1 ? argv[1] : "../bench/eval/typed_retrieval.json";
    const std::string dbPath    = argc > 2 ? argv[2] : "../database/typed_retrieval_test.db";

    // ---- 读基准 ----
    Json::Value root;
    {
        std::ifstream f(benchPath);
        if (!f.is_open()) { std::cerr << "打不开基准: " << benchPath << "\n"; return 2; }
        Json::CharReaderBuilder rb;
        std::string errs;
        if (!Json::parseFromStream(rb, f, &root, &errs)) {
            std::cerr << "基准解析失败: " << errs << "\n"; return 2;
        }
    }
    const Json::Value& corpus  = root["corpus"];
    const Json::Value& queries = root["queries"];
    if (!corpus.isArray() || !queries.isArray() || queries.empty()) {
        std::cerr << "基准格式不对（需 corpus + queries 数组）\n"; return 2;
    }

    // ---- 建库 ----
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }
    EdgeMemoryStore store(storage);
    if (!store.ready()) { std::cerr << "store 未就绪（[edge] 模型路径没配？）\n"; return 1; }

    // ---- 灌语料（corpus id → store id 映射；episodic 传 ts 作 valid_from）----
    std::map<int, int> idMap;
    for (const auto& mem : corpus) {
        MemoryType type = MemoryType::Semantic;
        memoryTypeFromName(mem["type"].asString(), type);
        const std::string validFrom = mem.get("ts", "").asString();
        auto r = store.ingestText(mem["text"].asString(), type, "user", "private",
                                  "asserted", "", 0, validFrom);
        idMap[mem["id"].asInt()] = r.id;
    }

    std::cout << "=== Q3 类型特化检索 vs 单一策略基线 ===\n"
              << "  corpus=" << corpus.size() << "  queries=" << queries.size()
              << "  embed=" << store.embedModelTag()
              << "  kTypeBoost=" << EdgeMemoryStore::kTypeBoost
              << "  tau=" << EdgeMemoryStore::kEpisodicTauDays << "天\n\n";

    int baseHit = 0, typedHit = 0, intentOk = 0;
    const int nq = static_cast<int>(queries.size());
    std::map<std::string, int> baseByIntent, typedByIntent, totalByIntent;

    std::cout << "  逐查询（base=纯余弦 top1 / typed=类型特化 top1）:\n";
    for (const auto& q : queries) {
        const std::string query = q["query"].asString();
        const std::string intentLabel = q["intent"].asString();
        std::set<int> expected;
        for (const auto& e : q["expected_ids"]) expected.insert(idMap[e.asInt()]);

        // 意图识别
        const MemoryType detected = EdgeMemoryStore::detectQueryIntent(query);
        MemoryType labeled = MemoryType::Semantic;
        memoryTypeFromName(intentLabel, labeled);
        const bool intentCorrect = (detected == labeled);
        if (intentCorrect) ++intentOk;

        // 基线 vs 类型特化
        auto baseHits  = store.recall(query, 1);
        auto typedHits = store.recallTyped(query, 1);
        const bool baseOk  = !baseHits.empty()  && expected.count(baseHits[0].id);
        const bool typedOk = !typedHits.empty() && expected.count(typedHits[0].id);
        if (baseOk)  ++baseHit;
        if (typedOk) ++typedHit;
        totalByIntent[intentLabel]++;
        if (baseOk)  baseByIntent[intentLabel]++;
        if (typedOk) typedByIntent[intentLabel]++;

        std::string mark;
        if (typedOk && !baseOk)      mark = "↑修复";
        else if (!typedOk && baseOk) mark = "↓退化";
        else if (typedOk && baseOk)  mark = "=都对";
        else                         mark = "=都错";

        std::cout << "    " << mark << " [" << intentLabel
                  << (intentCorrect ? "" : " !意图误判为" ) << "]"
                  << (intentCorrect ? "" : memoryTypeName(detected)) << " " << query << "\n"
                  << "         base→" << (baseHits.empty() ? -1 : baseHits[0].id)
                  << (baseOk ? "✓" : "✗")
                  << "  typed→" << (typedHits.empty() ? -1 : typedHits[0].id)
                  << (typedOk ? "✓" : "✗") << "  expected={";
        for (auto it = expected.begin(); it != expected.end(); ++it)
            std::cout << (it == expected.begin() ? "" : ",") << *it;
        std::cout << "}\n";
    }

    // ---- 汇总 ----
    std::cout << "\n=== 汇总 ===\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  意图识别准确率:  " << intentOk << "/" << nq << " = "
              << 100.0 * intentOk / nq << "%\n";
    std::cout << "  基线 hit@1:      " << baseHit << "/" << nq << " = "
              << 100.0 * baseHit / nq << "%  （纯余弦，单一策略）\n";
    std::cout << "  类型特化 hit@1:  " << typedHit << "/" << nq << " = "
              << 100.0 * typedHit / nq << "%\n";
    const double delta = 100.0 * (typedHit - baseHit) / nq;
    std::cout << "  提升:            " << (delta >= 0 ? "+" : "") << delta << " 个百分点\n";

    std::cout << "\n  分意图 hit@1（基线 → 类型特化）:\n";
    for (const auto& kv : totalByIntent) {
        const std::string& it = kv.first;
        std::cout << "    " << it;
        for (size_t i = it.size(); i < 12; ++i) std::cout << " ";
        std::cout << baseByIntent[it] << "/" << kv.second << " → "
                  << typedByIntent[it] << "/" << kv.second << "\n";
    }

    std::cout << "\n=== 结果: 基线 " << baseHit << "/" << nq
              << " → 类型特化 " << typedHit << "/" << nq << " ===\n";
    return 0;   // 评测工具：靠打印的提升判断，不返回非零
}
