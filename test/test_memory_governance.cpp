#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <sqlite3.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "memory/core/MemoryStore.h"

// Q7 验证：治理三元标签过滤（owner/scope/authority）+ 强度衰减淘汰。
// 用法: test_memory_governance [db路径]
// 前置：config.ini 的 [edge] 段须配好模型与分词器路径（ingest 需算 embedding）。

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const std::string& name, const std::string& detail = "") {
    if (cond) { ++g_pass; std::cout << "  [PASS] " << name; }
    else { ++g_fail; std::cout << "  [FAIL] " << name; }
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "../database/governance_test.db";
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }

    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    MemoryStore store(storage);
    std::cout << "=== Q7 治理标签过滤 + 强度衰减淘汰 验证 ===" << std::endl;
    std::cout << "  db=" << dbPath << "\n  embed_model=" << store.embedModelTag() << std::endl;
    check(store.ready(), "store ready");

    auto clearAll = [&]() {
        for (const auto& m : store.list(std::nullopt, 0, /*includeInvalid=*/true))
            store.remove(m.id);
    };

    // ------------------------------------------------------------------
    // 1. 治理三元标签过滤（§8）
    // ------------------------------------------------------------------
    std::cout << "\n-- 治理标签过滤 --" << std::endl;
    auto r1 = store.ingestText("用户喜欢深蓝色的车", MemoryType::Semantic,
                               "user", "private", "asserted");
    auto r2 = store.ingestText("团队共享会议室在 3 楼", MemoryType::Semantic,
                               "system", "shared", "asserted");
    auto r3 = store.ingestText("agent 观察到用户今天心情不好", MemoryType::Semantic,
                               "agent", "private", "observed");
    check(r1.action == MemoryAction::Created && r2.action == MemoryAction::Created &&
          r3.action == MemoryAction::Created, "写入 3 条不同治理标签的记忆");

    check(store.list().size() == 3, "list() 无过滤 == 3",
          "实际 " + std::to_string(store.list().size()));

    GovernanceFilter govPrivate; govPrivate.scope = "private";
    auto lp = store.list(std::nullopt, 0, false, govPrivate);
    check(lp.size() == 2, "list scope=private == 2（r1+r3）", "实际 " + std::to_string(lp.size()));

    GovernanceFilter govShared; govShared.scope = "shared";
    auto ls = store.list(std::nullopt, 0, false, govShared);
    check(ls.size() == 1 && ls[0].id == r2.id, "list scope=shared == r2");

    GovernanceFilter govUser; govUser.owner = "user";
    auto lu = store.list(std::nullopt, 0, false, govUser);
    check(lu.size() == 1 && lu[0].id == r1.id, "list owner=user == r1");

    GovernanceFilter govAgent; govAgent.owner = "agent";
    auto la = store.list(std::nullopt, 0, false, govAgent);
    check(la.size() == 1 && la[0].id == r3.id, "list owner=agent == r3");

    GovernanceFilter govObs; govObs.authority = "observed";
    auto lo = store.list(std::nullopt, 0, false, govObs);
    check(lo.size() == 1 && lo[0].id == r3.id, "list authority=observed == r3");

    GovernanceFilter govCombo; govCombo.owner = "user"; govCombo.scope = "private";
    auto lc = store.list(std::nullopt, 0, false, govCombo);
    check(lc.size() == 1 && lc[0].id == r1.id, "list owner=user+scope=private == r1");

    GovernanceFilter govNone; govNone.owner = "user"; govNone.scope = "shared";
    auto ln = store.list(std::nullopt, 0, false, govNone);
    check(ln.empty(), "list owner=user+scope=shared == 空（无此组合）",
          "实际 " + std::to_string(ln.size()));

    // recall 也走治理过滤：scope=shared 时不应召回 private 的 r1
    auto recShared = store.recall("车的颜色偏好", 4, std::nullopt, false, govShared);
    bool noPrivate = true;
    for (const auto& m : recShared) if (m.id == r1.id) noPrivate = false;
    check(noPrivate, "recall scope=shared 不含 private 记忆 r1");

    clearAll();

    // ------------------------------------------------------------------
    // 2. 强度衰减淘汰（§9）：类型优先级 episodic 先 → semantic → procedural 后
    // ------------------------------------------------------------------
    std::cout << "\n-- 强度淘汰：类型优先级 --" << std::endl;
    auto e1 = store.ingestText("上次充电是在 A 充电站", MemoryType::Episodic);
    auto s1 = store.ingestText("用户喜欢红色", MemoryType::Semantic);
    auto p1 = store.ingestText("启动步骤：先按 A 再按 B", MemoryType::Procedural);
    check(e1.action == MemoryAction::Created && s1.action == MemoryAction::Created &&
          p1.action == MemoryAction::Created, "写入 episodic/semantic/procedural 各 1 条");

    int n1 = store.evictToCapacity(2);   // 3→2，淘汰优先级最低的 episodic
    check(n1 == 1 && store.get(e1.id).id == 0, "evictToCapacity(2) 淘汰 episodic e1",
          "evicted=" + std::to_string(n1));
    check(store.get(s1.id).id == s1.id && store.get(p1.id).id == p1.id,
          "semantic s1 与 procedural p1 仍在");

    int n2 = store.evictToCapacity(1);   // 2→1，淘汰 semantic（优先级次于 procedural）
    check(n2 == 1 && store.get(s1.id).id == 0, "evictToCapacity(1) 再淘汰 semantic s1",
          "evicted=" + std::to_string(n2));
    check(store.get(p1.id).id == p1.id, "procedural p1 最后淘汰（技能不该被忘）");

    int n3 = store.evictToCapacity(0);   // 1→0
    check(n3 == 1 && store.count() == 0, "evictToCapacity(0) 清空", "evicted=" + std::to_string(n3));

    clearAll();

    // ------------------------------------------------------------------
    // 3. 强度淘汰：同类型内有效强度最低者优先（reinforce 提升强度 ⇒ 更耐淘汰）
    // ------------------------------------------------------------------
    std::cout << "\n-- 强度淘汰：同类型比强度 --" << std::endl;
    auto sa = store.ingestText("用户住在杭州西湖区", MemoryType::Semantic);
    auto sb = store.ingestText("用户的狗叫豆豆", MemoryType::Semantic);
    store.reinforce(sa.id, 5.0f);   // sa 强度 1→6，sb 仍 1
    int n4 = store.evictToCapacity(1);   // 2→1，淘汰更弱的 sb
    check(n4 == 1 && store.get(sb.id).id == 0, "evictToCapacity(1) 淘汰未强化的 sb",
          "evicted=" + std::to_string(n4));
    check(store.get(sa.id).id == sa.id, "被 reinforce 的 sa 更耐淘汰");

    clearAll();

    // ------------------------------------------------------------------
    // 4. 强度淘汰：时间衰减（updated_at 越旧 ⇒ 衰减越多 ⇒ 越先被淘汰）
    // ------------------------------------------------------------------
    std::cout << "\n-- 强度淘汰：时间衰减 --" << std::endl;
    auto ta = store.ingestText("用户最近开始学钢琴", MemoryType::Semantic);
    auto tb = store.ingestText("用户三年前养过一只猫", MemoryType::Semantic);
    // 把 tb 的 updated_at 回拨到 2020 年 ⇒ timeDecay 几乎为 0 ⇒ 有效强度 ≈ 0
    {
        sqlite3* db = storage.getDatabase();
        const std::string sql = "UPDATE memory SET updated_at = '2020-01-01 00:00:00' WHERE id = "
                                + std::to_string(tb.id);
        sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }
    int n5 = store.evictToCapacity(1);   // 2→1，淘汰衰减更厉害的 tb
    check(n5 == 1 && store.get(tb.id).id == 0, "evictToCapacity(1) 淘汰时间更久的 tb",
          "evicted=" + std::to_string(n5));
    check(store.get(ta.id).id == ta.id, "较新的 ta 衰减更少、留存");

    clearAll();

    std::cout << "\n=== 结果: PASS " << g_pass << " / FAIL " << g_fail << " ===" << std::endl;
    return 0;   // 评测工具恒 0，判据看 PASS/FAIL 计数
}
