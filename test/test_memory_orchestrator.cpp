#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/MemoryOrchestrator.h"
#include "legacy/infra-alt/ModelManager.h"

// P3 验证：MemoryOrchestrator 的写入 / 跨类型检索 / 敏感标记 / 实时写入 / 分类型 FIFO 淘汰。
//
// 用法: test_memory_orchestrator [corpus.json] [db路径]
// 默认 corpus=../bench/corpus/cockpit.json  db=../database/orchestrator_test.db
// （从 build-edge/ 运行，../ 指向仓库根；config.ini 的 [edge] 路径须已配置）

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const std::string& name, const std::string& detail = "") {
    if (cond) { ++g_pass; std::cout << "  [PASS] " << name; }
    else { ++g_fail; std::cout << "  [FAIL] " << name; }
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    const std::string corpus = argc > 1 ? argv[1] : "../bench/corpus/cockpit.json";
    const std::string dbPath = argc > 2 ? argv[2] : "../database/orchestrator_test.db";

    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }

    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    ModelManager model(storage);
    MemoryOrchestrator orch(storage, model);

    std::cout << "=== P3 MemoryOrchestrator 验证 ===" << std::endl;
    std::cout << "  corpus=" << corpus << "\n  db=" << dbPath << std::endl;

    // ---- 1. 语料加载 ----
    const int n = orch.loadCorpus(corpus);
    check(n == 40, "loadCorpus 写入 40 条", "实际 " + std::to_string(n));
    check(orch.countAll() == n, "countAll 与写入数一致", "countAll=" + std::to_string(orch.countAll()));

    // ---- 2. 跨类型检索：颜色偏好 ----
    {
        auto hits = orch.retrieve("我的车是什么颜色偏好？", 4);
        check(!hits.empty(), "检索返回非空");
        if (!hits.empty()) {
            const bool ok = hits[0].content.find("深蓝") != std::string::npos;
            check(ok, "top-1 命中「深蓝」", "top-1=" + hits[0].content.substr(0, 30));
            std::cout << "        score=" << hits[0].score << " type=" << hits[0].type
                      << " 命中数=" << hits.size() << std::endl;
        }
    }

    // ---- 3. 敏感标记传播（隐私路由的依据）----
    {
        auto hits = orch.retrieve("我住在哪里？", 4);
        bool any_sensitive = false;
        for (const auto& h : hits) if (h.sensitive) any_sensitive = true;
        check(any_sensitive, "「我住在哪里」命中含 sensitive=true");
        const bool top_addr = !hits.empty() && hits[0].content.find("西湖区") != std::string::npos;
        check(top_addr, "top-1 是住址记忆",
              hits.empty() ? "" : hits[0].content.substr(0, 30));
    }

    // ---- 4. 实时写入 + 可被检索 ----
    {
        const bool ok = orch.write("episodic", "用户今天去了莫干山自驾露营", false, "{\"ts\":\"2026-10-20\"}");
        check(ok, "write 新记忆成功");
        auto hits = orch.retrieve("我最近去哪里玩了？", 3);
        bool found = false;
        for (const auto& h : hits) if (h.content.find("莫干山") != std::string::npos) found = true;
        check(found, "新写入的记忆可被检索到");
    }

    // ---- 5. 分类型有界淘汰（FIFO 淘汰最旧、保留最新）----
    {
        const int before = orch.countByType("episodic");
        orch.setTypeCapacity("episodic", 3);
        for (int i = 0; i < 5; ++i)
            orch.write("episodic", "淘汰测试 episodic " + std::to_string(i), false, "{}");
        const int after = orch.countByType("episodic");
        check(after == 3, "episodic 淘汰到容量 3",
              "before=" + std::to_string(before) + " after=" + std::to_string(after));

        auto hits = orch.retrieve("淘汰测试 episodic 4", 5);
        bool kept_newest = false;
        for (const auto& h : hits) if (h.content.find("episodic 4") != std::string::npos) kept_newest = true;
        check(kept_newest, "FIFO 保留最新（episodic 4 仍在）");

        // 其它类型不受影响（分类型隔离）
        check(orch.countByType("preference") > 3, "preference 类型未被 episodic 的容量影响",
              "preference=" + std::to_string(orch.countByType("preference")));
    }

    std::cout << "\n=== 结果: PASS " << g_pass << " / FAIL " << g_fail << " ===" << std::endl;
    return g_fail == 0 ? 0 : 1;
}
