#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <iomanip>
#include <json/json.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/MemoryOrchestrator.h"
#include "legacy/infra-alt/ModelManager.h"

// P4 三域检索评测：加载语料 → 跑带标注查询 → 算 recall@k / MRR / hit@1。
//
// 评测对象是**记忆系统的检索质量**，与 LLM 生成解耦（不需要 llama-server）。
// 同时把每条查询的 retrieved id 列表写进 out.json，供 INT8 vs FP32 的
// top-k 命中集合一致性对比（bench/compare_topk.py）。
//
// 用法: test_retrieval_eval <corpus.json> <eval.json> <db_path> <out.json> [topk]
int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "usage: " << argv[0]
                  << " <corpus.json> <eval.json> <db_path> <out.json> [topk]" << std::endl;
        return 1;
    }
    const std::string corpus = argv[1], evalPath = argv[2];
    const std::string dbPath = argv[3], outPath = argv[4];

    // ---- 读评测集 ----
    Json::CharReaderBuilder rb;
    std::string errs;
    Json::Value evalRoot;
    {
        std::ifstream ef(evalPath);
        if (!ef.is_open()) { std::cerr << "cannot open eval: " << evalPath << "\n"; return 1; }
        if (!Json::parseFromStream(rb, ef, &evalRoot, &errs)) {
            std::cerr << "eval JSON parse failed: " << errs << "\n"; return 1;
        }
    }
    const std::string domain = evalRoot.get("domain", "?").asString();
    const int topk = argc > 5 ? std::atoi(argv[5]) : evalRoot.get("topk", 4).asInt();
    const Json::Value& queries = evalRoot["queries"];
    if (!queries.isArray() || queries.empty()) { std::cerr << "eval has no queries\n"; return 1; }

    // ---- 建库 + 加载语料 ----
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }
    ModelManager model(storage);
    MemoryOrchestrator orch(storage, model);

    const int loaded = orch.loadCorpus(corpus);
    if (loaded == 0) { std::cerr << "corpus loaded 0 memories\n"; return 1; }

    // ---- 逐条评测 ----
    double sumRecall = 0.0, sumRR = 0.0;
    int hit1 = 0, perfect = 0;
    Json::Value outQueries(Json::arrayValue);

    std::cout << "\n=== 域: " << domain << "  corpus=" << loaded
              << "  queries=" << queries.size() << "  topk=" << topk << " ===" << std::endl;

    for (const auto& q : queries) {
        const std::string query = q["query"].asString();
        std::set<int> expected;
        for (const auto& e : q["expected_ids"]) expected.insert(e.asInt());

        std::vector<MemoryHit> hits = orch.retrieve(query, topk);
        std::vector<int> retrieved;
        for (const auto& h : hits) retrieved.push_back(h.id);

        // recall@k = |retrieved ∩ expected| / |expected|
        int found = 0;
        for (int id : retrieved) if (expected.count(id)) ++found;
        const double recall = expected.empty() ? 0.0
                              : static_cast<double>(found) / static_cast<double>(expected.size());

        // RR = 1 / (第一个命中的排名)
        double rr = 0.0;
        for (size_t i = 0; i < retrieved.size(); ++i) {
            if (expected.count(retrieved[i])) { rr = 1.0 / static_cast<double>(i + 1); break; }
        }
        if (rr == 1.0) ++hit1;
        if (found == static_cast<int>(expected.size()) && !expected.empty()) ++perfect;

        sumRecall += recall;
        sumRR += rr;

        // 打印
        std::cout << "  " << (recall >= 1.0 ? "[OK]  " : (recall > 0 ? "[PART]" : "[MISS]"))
                  << " " << query << "\n"
                  << "         expected={";
        for (auto it = expected.begin(); it != expected.end(); ++it)
            std::cout << (it == expected.begin() ? "" : ",") << *it;
        std::cout << "}  retrieved={";
        for (size_t i = 0; i < retrieved.size(); ++i)
            std::cout << (i ? "," : "") << retrieved[i];
        std::cout << "}  recall=" << std::fixed << std::setprecision(2) << recall
                  << " rr=" << rr << std::endl;

        Json::Value oq;
        oq["query"] = query;
        Json::Value expArr(Json::arrayValue), retArr(Json::arrayValue);
        for (int id : expected) expArr.append(id);
        for (int id : retrieved) retArr.append(id);
        oq["expected"] = expArr;
        oq["retrieved"] = retArr;
        oq["recall"] = recall;
        oq["rr"] = rr;
        outQueries.append(oq);
    }

    const int nq = static_cast<int>(queries.size());
    const double recallAtK = sumRecall / nq;
    const double mrr = sumRR / nq;
    const double hitAt1 = static_cast<double>(hit1) / nq;
    const double perfectRate = static_cast<double>(perfect) / nq;

    std::cout << "\n  --- " << domain << " 汇总 ---" << std::endl;
    std::cout << "  recall@" << topk << " = " << std::fixed << std::setprecision(4) << recallAtK << std::endl;
    std::cout << "  MRR        = " << mrr << std::endl;
    std::cout << "  hit@1      = " << hitAt1 << "  (" << hit1 << "/" << nq << ")" << std::endl;
    std::cout << "  全命中率    = " << perfectRate << "  (" << perfect << "/" << nq << ")" << std::endl;

    // ---- 写结果 JSON ----
    Json::Value out;
    out["domain"] = domain;
    out["topk"] = topk;
    out["n_queries"] = nq;
    out["n_corpus"] = loaded;
    out["recall_at_k"] = recallAtK;
    out["mrr"] = mrr;
    out["hit_at_1"] = hitAt1;
    out["perfect_rate"] = perfectRate;
    out["queries"] = outQueries;

    std::ofstream of(outPath);
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "  ";
    of << Json::writeString(wb, out);
    std::cout << "  写入 " << outPath << std::endl;
    return 0;
}
