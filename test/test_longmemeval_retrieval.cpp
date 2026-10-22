#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <set>
#include <iomanip>
#include <cstdlib>
#include <sqlite3.h>
#include <json/json.h>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/EdgeMemoryStore.h"

// ============================================================================
// Q5 验证：LongMemEval turn 级检索评测（recall@k / MRR），用英文模型跑。
// ----------------------------------------------------------------------------
// 核心主张（docs/memory_v2_design.md §12.1）：同一份 C++ 二进制、零代码改动，
// 只把 config.ini 的 [edge] 模型路径从 bge-small-zh 换成 bge-small-en-v1.5，
// 就能在英文 LongMemEval 上做检索。本测试即物证——它不碰任何语言相关代码。
//
// 金标：LongMemEval 自带 turn 级 has_answer 标注（哪条 turn 含答案），
// 故检索评测**不需要 LLM**（与本项目「评测与生成解耦」原则一致）。
//
// 方法：每题把 haystack 的每条 turn 存成一条记忆 → 用 question 检索 top-k →
// 看 has_answer 的 turn 是否被召回。recall@k = 命中 has_answer 数 / has_answer 总数；
// MRR = 第一条 has_answer 的倒数排名。
//
// 用法: test_longmemeval_retrieval [data.json] [db路径] [每题型题数] [topK]
// 前置：config.ini 的 [edge] 段指向英文模型（bge-small-en-v1.5）。
// ============================================================================

int main(int argc, char** argv) {
    const std::string dataPath = argc > 1 ? argv[1] : "../bench/data/longmemeval_oracle.json";
    const std::string dbPath   = argc > 2 ? argv[2] : "../database/longmemeval_test.db";
    const int nPerType         = argc > 3 ? std::atoi(argv[3]) : 5;
    const int topK             = argc > 4 ? std::atoi(argv[4]) : 10;

    // ---- 读 LongMemEval ----
    Json::Value data;
    {
        std::ifstream f(dataPath);
        if (!f.is_open()) { std::cerr << "打不开数据: " << dataPath << "\n"; return 2; }
        Json::CharReaderBuilder rb;
        std::string errs;
        if (!Json::parseFromStream(rb, f, &data, &errs)) {
            std::cerr << "解析失败: " << errs << "\n"; return 2;
        }
    }
    if (!data.isArray()) { std::cerr << "数据格式不对（需数组）\n"; return 2; }

    // ---- 建库 + store（走 config.ini 的英文模型）----
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }
    EdgeMemoryStore store(storage);
    if (!store.ready()) { std::cerr << "store 未就绪（[edge] 模型路径没配？）\n"; return 1; }

    std::cout << "=== Q5 LongMemEval turn 级检索评测 ===\n"
              << "  embed model = " << store.embedModelTag()
              << "  dim = " << store.dim()
              << "  topK = " << topK << "\n";

    // ---- 分层子集（每题型取前 N，确定性可复现）----
    std::map<std::string, int> typeCount;
    std::vector<const Json::Value*> subset;
    for (const auto& q : data) {
        const std::string qt = q["question_type"].asString();
        if (typeCount[qt] < nPerType) { subset.push_back(&q); typeCount[qt]++; }
    }
    std::cout << "  子集 = " << subset.size() << " 题（每题型前 " << nPerType << "）\n\n";

    // ---- 评测 ----
    double sumRecall = 0, sumMRR = 0;
    int nQ = 0, skipped = 0;
    std::map<std::string, double> typeRecall, typeMRR;
    std::map<std::string, int> typeN;

    for (const auto* qp : subset) {
        const Json::Value& q = *qp;
        const std::string qtype = q["question_type"].asString();

        // 清空记忆表（每题独立 haystack）
        sqlite3_exec(storage.getDatabase(), "DELETE FROM memory;", nullptr, nullptr, nullptr);

        // 存每条 turn，记录 has_answer 的记忆 id
        std::set<int> answerIds;
        int nTurns = 0;
        for (const auto& sess : q["haystack_sessions"]) {
            for (const auto& turn : sess) {
                const std::string content = turn["content"].asString();
                if (content.empty()) continue;
                const bool ha = turn.get("has_answer", false).asBool();
                auto r = store.ingestText(content, MemoryType::Semantic);
                if (ha && r.id > 0) answerIds.insert(r.id);
                nTurns++;
            }
        }
        if (answerIds.empty()) { skipped++; continue; }   // 无 has_answer 金标（如 _abs 题）跳过

        // 检索 top-k
        const std::string question = q["question"].asString();
        auto hits = store.recall(question, topK);

        // recall@k + MRR
        int retrieved = 0;
        double rr = 0;
        for (size_t i = 0; i < hits.size(); ++i) {
            if (answerIds.count(hits[i].id)) {
                retrieved++;
                if (rr == 0) rr = 1.0 / static_cast<double>(i + 1);
            }
        }
        const double recall = static_cast<double>(retrieved) / answerIds.size();
        sumRecall += recall;
        sumMRR += rr;
        typeRecall[qtype] += recall;
        typeMRR[qtype] += rr;
        typeN[qtype]++;
        nQ++;

        std::cout << "  [" << std::setw(2) << nQ << "] " << std::left << std::setw(24)
                  << qtype.substr(0, 24) << std::right
                  << " turns=" << std::setw(3) << nTurns
                  << " ans=" << answerIds.size()
                  << " recall@" << topK << "=" << std::fixed << std::setprecision(2) << recall
                  << " RR=" << std::setprecision(3) << rr << "\n";
    }

    // ---- 汇总 ----
    std::cout << "\n=== 汇总 ===\n" << std::setprecision(4);
    std::cout << "  评测题数: " << nQ << "（跳过无金标 " << skipped << "）\n";
    std::cout << "  turn-level recall@" << topK << ": " << (nQ ? sumRecall / nQ : 0) << "\n";
    std::cout << "  turn-level MRR:       " << (nQ ? sumMRR / nQ : 0) << "\n";
    std::cout << "\n  分题型（recall@" << topK << " / MRR / n）:\n";
    for (const auto& kv : typeN) {
        const std::string& t = kv.first;
        std::cout << "    " << std::left << std::setw(26) << t.substr(0, 26) << std::right
                  << std::setprecision(3) << typeRecall[t] / kv.second << "  "
                  << typeMRR[t] / kv.second << "  " << kv.second << "\n";
    }
    std::cout << "\n=== 结果: recall@" << topK << " " << std::setprecision(4)
              << (nQ ? sumRecall / nQ : 0) << " / MRR " << (nQ ? sumMRR / nQ : 0) << " ===\n";
    return 0;
}
