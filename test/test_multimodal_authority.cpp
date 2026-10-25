#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <sqlite3.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "memory/core/MemoryStore.h"
#include "memory/core/MemoryResolver.h"

// Q9 验证：authority 参与消解决策（此前 authority 只用于治理过滤，从不参与消解）
//          + source_image 视觉溯源列。
// 用法: test_multimodal_authority [db路径]
// 前置：A/B 两部分是纯函数、**无需模型**；C/D 需要 config.ini 的 [edge] 段配好 embedding 模型。
//
// 设计说明：authority 闸门刻意做成 public static 纯函数（applyAuthorityGate）——
// 无 LLM 时 System-2 回退 CREATE、闸门在 resolve() 里根本不会被触发，所以闸门逻辑
// 本身必须能离线独立验证。「System-2 产出 SUPERSEDE/UPDATE 后过闸门」的端到端路径
// 需要本地 LLM，属 M2 在 dog 上的实测部分（docs/q9_multimodal_plan.md §2）。

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const std::string& name, const std::string& detail = "") {
    if (cond) { ++g_pass; std::cout << "  [PASS] " << name; }
    else { ++g_fail; std::cout << "  [FAIL] " << name; }
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << std::endl;
}

static Memory makeTarget(int id, const std::string& authority) {
    Memory m;
    m.id = id;
    m.type = MemoryType::Semantic;
    m.content = "目标记忆";
    m.authority = authority;
    return m;
}

static ResolveDecision makeDecision(MemoryAction action, int targetId) {
    ResolveDecision d;
    d.action = action;
    d.targetId = targetId;
    d.similarity = 0.8f;
    return d;
}

// ---------------------------------------------------------------------------
// A. authorityRank：observed > asserted > inferred
// ---------------------------------------------------------------------------
static void testAuthorityRank() {
    std::cout << "\n-- A. authorityRank 优先级 --" << std::endl;
    check(MemoryResolver::authorityRank("observed") == 2, "A1 observed = 2");
    check(MemoryResolver::authorityRank("asserted") == 1, "A2 asserted = 1");
    check(MemoryResolver::authorityRank("inferred") == 0, "A3 inferred = 0");
    check(MemoryResolver::authorityRank("garbage") == 1, "A4 未知值按 asserted = 1");
    check(MemoryResolver::authorityRank("") == 1, "A5 空值按 asserted = 1");
    check(MemoryResolver::authorityRank("observed") > MemoryResolver::authorityRank("asserted") &&
          MemoryResolver::authorityRank("asserted") > MemoryResolver::authorityRank("inferred"),
          "A6 优先级链 observed > asserted > inferred");
}

// ---------------------------------------------------------------------------
// B. applyAuthorityGate：闸门真值表（纯函数，离线确定性）
// ---------------------------------------------------------------------------
static void testAuthorityGate() {
    std::cout << "\n-- B. applyAuthorityGate 闸门真值表 --" << std::endl;

    // ---- 同 authority → 完全不介入（M3 硬闸门：Q2 五操作行为逐条不变）----
    {
        auto d = makeDecision(MemoryAction::Superseded, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "asserted"), "asserted");
        check(d.action == MemoryAction::Superseded && d.targetId == 7 &&
              !d.blockedByAuthority && !d.authorityPromoted,
              "B1 同 asserted + SUPERSEDE → 不变");
    }
    {
        auto d = makeDecision(MemoryAction::Updated, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "asserted"), "asserted");
        check(d.action == MemoryAction::Updated && d.targetId == 7 && !d.blockedByAuthority,
              "B2 同 asserted + UPDATE → 不变");
    }
    {
        auto d = makeDecision(MemoryAction::Superseded, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "observed");
        check(d.action == MemoryAction::Superseded && !d.blockedByAuthority,
              "B3 同 observed + SUPERSEDE → 不变");
    }

    // ---- 候选 authority 更高 → 胜出 ----
    {
        auto d = makeDecision(MemoryAction::Superseded, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "asserted"), "observed");
        check(d.action == MemoryAction::Superseded && d.targetId == 7 && !d.authorityPromoted,
              "B4 observed SUPERSEDE asserted → 保持 SUPERSEDE（本就正确，不需升级）");
    }
    {
        auto d = makeDecision(MemoryAction::Updated, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "asserted"), "observed");
        check(d.action == MemoryAction::Superseded && d.authorityPromoted && d.targetId == 7,
              "B5 observed UPDATE asserted → 升级为 SUPERSEDE + authorityPromoted=true");
    }

    // ---- 候选 authority 更低 → 拦下（Q9 非对称核心：低不得静默覆盖高）----
    {
        auto d = makeDecision(MemoryAction::Superseded, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Created && d.blockedByAuthority &&
              d.targetId == 0 && d.blockedTargetId == 7,
              "B6 asserted SUPERSEDE observed → 拦下为并存 CREATE；targetId 清零、blockedTargetId 记住 7");
    }
    {
        auto d = makeDecision(MemoryAction::Updated, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Created && d.blockedByAuthority && d.blockedTargetId == 7,
              "B7 asserted UPDATE observed → 拦下为并存 CREATE");
    }

    // ---- inferred 最低 ----
    {
        auto d = makeDecision(MemoryAction::Superseded, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "asserted"), "inferred");
        check(d.action == MemoryAction::Created && d.blockedByAuthority,
              "B8 inferred SUPERSEDE asserted → 拦下（inferred < asserted）");
    }
    {
        auto d = makeDecision(MemoryAction::Updated, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "inferred"), "asserted");
        check(d.action == MemoryAction::Superseded && d.authorityPromoted,
              "B9 asserted UPDATE inferred → 升级（asserted > inferred）");
    }

    // ---- 闸门不该碰的动作 ----
    {
        auto d = makeDecision(MemoryAction::Reinforced, 7);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Reinforced && !d.blockedByAuthority,
              "B10 asserted REINFORCE observed → 不改（等价重述非冲突）");
    }
    {
        auto d = makeDecision(MemoryAction::Noop, 0);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Noop && !d.blockedByAuthority, "B11 NOOP → 不介入");
    }
    {
        auto d = makeDecision(MemoryAction::Created, 0);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Created && !d.blockedByAuthority,
              "B12 targetId=0（无目标）→ 不介入");
    }
    {
        auto d = makeDecision(MemoryAction::Superseded, 99);
        MemoryResolver::applyAuthorityGate(d, makeTarget(7, "observed"), "asserted");
        check(d.action == MemoryAction::Superseded && d.targetId == 99 && !d.blockedByAuthority,
              "B13 targetId 与 target.id 不匹配 → 不介入（防误伤）");
    }
}

// ---------------------------------------------------------------------------
// C. 存储路径：source_image 溯源列 + supersede 的 authority 覆盖
// ---------------------------------------------------------------------------
static void testStorePath(StorageManager& storage, MemoryStore& store) {
    std::cout << "\n-- C. source_image 溯源列 + supersede authority 覆盖 --" << std::endl;

    {   // C1 schema 含 source_image 列
        sqlite3_stmt* st = nullptr;
        bool has = false;
        if (sqlite3_prepare_v2(storage.getDatabase(), "PRAGMA table_info(memory);",
                               -1, &st, nullptr) == SQLITE_OK) {
            while (sqlite3_step(st) == SQLITE_ROW) {
                const unsigned char* n = sqlite3_column_text(st, 1);
                if (n && std::string(reinterpret_cast<const char*>(n)) == "source_image") {
                    has = true; break;
                }
            }
            sqlite3_finalize(st);
        }
        check(has, "C1 schema 含 source_image 列");
    }

    const auto r1 = store.ingestText("车辆停在 B2 层 234 号车位", MemoryType::Episodic,
                                     "张三", "private", "observed", "cockpit-cam", 1,
                                     /*validFrom=*/"", "/frames/parking_001.jpg");
    check(r1.action == MemoryAction::Created && r1.id > 0, "C2 observed 记忆写入（带 source_image）");

    const Memory m1 = store.get(r1.id);
    check(m1.sourceImage == "/frames/parking_001.jpg", "C3 source_image 读回一致",
          "实际「" + m1.sourceImage + "」");
    check(m1.authority == "observed", "C4 authority=observed 落库");

    const auto r2 = store.ingestText("用户说车停在 B1 层", MemoryType::Episodic,
                                     "张三", "private", "asserted");
    check(r2.action == MemoryAction::Created && store.get(r2.id).sourceImage.empty(),
          "C5 不传 source_image 时为空（向后兼容）");

    // C6–C10：observed 取代 asserted，新记录必须带 observed（不能继承 asserted）
    int newId = 0;
    const bool ok = store.supersede(r2.id, "摄像头观察到车辆实际停在 B2 层", &newId,
                                    "observed", "/frames/parking_002.jpg");
    check(ok && newId > 0, "C6 supersede 成功");
    const Memory oldM = store.get(r2.id);
    const Memory newM = store.get(newId);
    check(newM.authority == "observed",
          "C7 新记录 authority=observed（**未继承旧记录的 asserted**）",
          "实际「" + newM.authority + "」");
    check(newM.sourceImage == "/frames/parking_002.jpg", "C8 新记录带 source_image");
    check(!oldM.valid && oldM.supersededBy == newId, "C9 旧记录失效 + superseded_by 指向新记录");
    check(newM.supersedes == r2.id, "C10 新记录 supersedes 指向旧记录");

    // C11：不传 override → 继承旧 authority（Q1–Q7 行为不变）
    const auto r3 = store.ingestText("空调偏好 22 度", MemoryType::Semantic,
                                     "张三", "private", "asserted");
    int newId3 = 0;
    store.supersede(r3.id, "空调偏好改成 24 度", &newId3);
    check(newId3 > 0 && store.get(newId3).authority == "asserted",
          "C11 不传 override 时继承旧 authority=asserted（向后兼容）");

    const auto hist = store.history(newId);
    check(hist.size() == 2, "C12 history() 回溯到 2 条（旧+新）",
          "实际 " + std::to_string(hist.size()));

    bool foundImg = false;
    for (const auto& m : store.list(std::nullopt, 0, /*includeInvalid=*/true))
        if (m.id == r1.id && m.sourceImage == "/frames/parking_001.jpg") foundImg = true;
    check(foundImg, "C13 list() 结果带 source_image");
}

// ---------------------------------------------------------------------------
// D. resolver.ingest 透传（无 LLM：System-2 回退 CREATE，闸门不触发）
// ---------------------------------------------------------------------------
static void testResolverPath(MemoryStore& store) {
    std::cout << "\n-- D. resolver.ingest 透传 sourceImage（无 LLM）--" << std::endl;
    MemoryResolver resolver(store, /*llm=*/nullptr);

    const auto r = resolver.ingest("摄像头观察到副驾放着一个黑色背包", MemoryType::Semantic,
                                   "张三", "private", "observed", "cockpit-cam", 2,
                                   "/frames/cabin_001.jpg");
    check(r.action == MemoryAction::Created && r.id > 0, "D1 无 LLM 时模糊地带回退 CREATE");
    const Memory m = store.get(r.id);
    check(m.sourceImage == "/frames/cabin_001.jpg", "D2 sourceImage 经 resolver 透传落库",
          "实际「" + m.sourceImage + "」");
    check(m.authority == "observed", "D3 authority=observed 经 resolver 透传落库");
    check(!r.blockedByAuthority, "D4 无冲突时 blockedByAuthority=false");
}

int main(int argc, char** argv) {
    std::cout << "=== Q9 authority 感知消解 + source_image 溯源 验证 ===" << std::endl;

    // A/B 是纯函数，不依赖数据库与模型，先跑（即使没配模型也能验证闸门逻辑）
    testAuthorityRank();
    testAuthorityGate();

    const std::string dbPath = argc > 1 ? argv[1] : "../database/multimodal_test.db";
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }

    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    MemoryStore store(storage);
    std::cout << "\n  db=" << dbPath << "\n  embed_model=" << store.embedModelTag() << std::endl;
    if (!store.ready()) {
        std::cerr << "store not ready（embedding 模型未配好）——C/D 部分跳过，"
                     "A/B 纯函数结果仍有效\n";
    } else {
        testStorePath(storage, store);
        testResolverPath(store);
    }

    std::cout << "\n=== 结果: " << g_pass << " PASS / " << g_fail << " FAIL ===" << std::endl;
    return g_fail == 0 ? 0 : 1;
}
