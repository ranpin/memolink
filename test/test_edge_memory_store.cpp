#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/EdgeMemoryStore.h"

// Q1 验证：EdgeMemoryStore 的基础 CRUD + 非破坏性版本化。
// 用法: test_edge_memory_store [db路径]
// 前置：config.ini 的 [edge] 段须配好模型与分词器路径（否则 embedding 会走云端/模拟回退）

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const std::string& name, const std::string& detail = "") {
    if (cond) { ++g_pass; std::cout << "  [PASS] " << name; }
    else { ++g_fail; std::cout << "  [FAIL] " << name; }
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "../database/edge_store_test.db";
    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }

    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }

    EdgeMemoryStore store(storage);
    std::cout << "=== Q1 EdgeMemoryStore 验证 ===" << std::endl;
    std::cout << "  db=" << dbPath << "\n  embed_model=" << store.embedModelTag() << std::endl;
    check(store.ready(), "store ready");
    check(store.dim() == 512, "embedding 维度 == 512", "实际 " + std::to_string(store.dim()));

    // ---- 1. 写入三类记忆 ----
    std::cout << "\n-- 写入 --" << std::endl;
    auto r1 = store.ingestText("用户喜欢深蓝色的车", MemoryType::Semantic, "user", "private");
    check(r1.action == MemoryAction::Created && r1.id > 0, "ingest semantic",
          "action=" + std::string(memoryActionName(r1.action)) + " id=" + std::to_string(r1.id));
    auto r2 = store.ingestText("上次保养是 2026 年 8 月，里程 12000 公里",
                               MemoryType::Episodic, "user", "shared");
    check(r2.action == MemoryAction::Created, "ingest episodic", "id=" + std::to_string(r2.id));
    auto r3 = store.ingestText("说「我有点冷」时应把空调调高两度并开座椅加热",
                               MemoryType::Procedural);
    check(r3.action == MemoryAction::Created, "ingest procedural", "id=" + std::to_string(r3.id));
    auto r4 = store.ingestText("用户住在杭州西湖区", MemoryType::Semantic, "user", "private");
    check(r4.action == MemoryAction::Created, "ingest 第 4 条", "id=" + std::to_string(r4.id));

    auto rBad = store.ingestText("", MemoryType::Semantic);
    check(rBad.action == MemoryAction::Failed, "空内容被拒绝",
          "reason=" + rBad.reason);

    // ---- 2. 计数与统计 ----
    std::cout << "\n-- 计数 --" << std::endl;
    check(store.count() == 4, "count() == 4", "实际 " + std::to_string(store.count()));
    check(store.count(MemoryType::Semantic) == 2, "semantic 计数 == 2",
          "实际 " + std::to_string(store.count(MemoryType::Semantic)));
    auto st = store.stats();
    check(st.total == 4 && st.valid == 4 && st.invalid == 0, "stats total/valid/invalid",
          "total=" + std::to_string(st.total) + " valid=" + std::to_string(st.valid) +
          " invalid=" + std::to_string(st.invalid));
    check(st.semantic == 2 && st.episodic == 1 && st.procedural == 1, "stats 分类型",
          "sem=" + std::to_string(st.semantic) + " epi=" + std::to_string(st.episodic) +
          " proc=" + std::to_string(st.procedural));

    // ---- 3. 检索排序 ----
    std::cout << "\n-- 检索 --" << std::endl;
    auto hits = store.recall("我的车是什么颜色偏好？", 4);
    check(!hits.empty(), "recall 非空", "命中 " + std::to_string(hits.size()) + " 条");
    if (!hits.empty()) {
        check(hits[0].id == r1.id, "top-1 命中「深蓝」记忆",
              "top1 id=" + std::to_string(hits[0].id) + " 期望 " + std::to_string(r1.id) +
              " score=" + std::to_string(hits[0].score));
        bool sorted = true;
        for (size_t i = 1; i < hits.size(); ++i)
            if (hits[i].score > hits[i - 1].score + 1e-6f) sorted = false;
        check(sorted, "结果按分数降序");
    }
    check(store.get(r1.id).accessCount >= 1, "命中后 access_count 递增",
          "实际 " + std::to_string(store.get(r1.id).accessCount));

    auto epiHits = store.recall("上次保养是什么时候？", 4, MemoryType::Episodic);
    check(!epiHits.empty() && epiHits[0].type == MemoryType::Episodic,
          "类型过滤只返回 episodic",
          epiHits.empty() ? "空" : std::string(memoryTypeName(epiHits[0].type)));

    // ---- 4. update ----
    std::cout << "\n-- update --" << std::endl;
    check(store.update(r1.id, "用户现在喜欢红色的车"), "update 返回 true");
    auto m1 = store.get(r1.id);
    check(m1.content == "用户现在喜欢红色的车", "内容已改", "实际: " + m1.content);
    auto afterUpd = store.recall("我的车是什么颜色偏好？", 1);
    check(!afterUpd.empty() && afterUpd[0].id == r1.id,
          "update 后 embedding 已重算（仍能检索到）");
    check(!store.update(99999, "不存在的 id"), "update 不存在的 id 返回 false");

    // ---- 5. supersede（非破坏性版本化）----
    std::cout << "\n-- supersede --" << std::endl;
    check(store.supersede(r4.id, "用户搬到上海浦东了"), "supersede 返回 true");
    auto oldM = store.get(r4.id);
    check(!oldM.valid, "旧记录已失效", "validUntil=" + oldM.validUntil);
    check(oldM.supersededBy > 0, "旧记录 superseded_by 已填",
          "值=" + std::to_string(oldM.supersededBy));
    auto newM = store.get(oldM.supersededBy);
    check(newM.id != 0, "新记录存在", "id=" + std::to_string(newM.id));
    check(newM.supersedes == r4.id, "新记录 supersedes 指回旧记录",
          "值=" + std::to_string(newM.supersedes));
    check(newM.content == "用户搬到上海浦东了", "新记录内容正确", "实际: " + newM.content);
    check(newM.valid, "新记录当前有效");
    check(newM.scope == oldM.scope && newM.owner == oldM.owner && newM.type == oldM.type,
          "supersede 继承 owner/scope/type");

    bool hasOld = false;
    for (const auto& m : store.list()) if (m.id == r4.id) hasOld = true;
    check(!hasOld, "默认 list 不含已失效记录");

    bool foundOld = false;
    for (const auto& m : store.list(std::nullopt, 0, true)) if (m.id == r4.id) foundOld = true;
    check(foundOld, "list(includeInvalid=true) 能看到已失效记录");

    auto histHits = store.recall("用户住在哪里？", 4, std::nullopt, false);
    bool leakedOld = false;
    for (const auto& m : histHits) if (m.id == r4.id) leakedOld = true;
    check(!leakedOld, "默认 recall 不返回已失效记录");

    // ---- 6. history ----
    std::cout << "\n-- history --" << std::endl;
    auto hist = store.history(r4.id);
    check(hist.size() == 2, "history 返回完整演化链（2 版）",
          "实际 " + std::to_string(hist.size()));
    if (hist.size() == 2) {
        check(hist[0].id == r4.id && hist[1].id == newM.id, "history 顺序为旧→新",
              "hist[0]=" + std::to_string(hist[0].id) + " hist[1]=" + std::to_string(hist[1].id));
        check(!hist[0].valid && hist[1].valid, "旧版失效、新版有效");
        check(hist[0].content == "用户住在杭州西湖区", "旧版内容仍可查",
              "实际: " + hist[0].content);
    }

    // ---- 7. remove ----
    std::cout << "\n-- remove --" << std::endl;
    const int beforeRemove = store.count(std::nullopt, false);
    check(store.remove(r2.id), "remove 返回 true");
    check(store.get(r2.id).id == 0, "remove 后 get 不到");
    check(store.count(std::nullopt, false) == beforeRemove - 1, "总数减一",
          "before=" + std::to_string(beforeRemove) +
          " after=" + std::to_string(store.count(std::nullopt, false)));
    check(!store.remove(99999), "remove 不存在的 id 返回 false");

    // ---- 8. 终态复核 ----
    std::cout << "\n-- 终态 --" << std::endl;
    auto st2 = store.stats();
    std::cout << "  total=" << st2.total << " valid=" << st2.valid << " invalid=" << st2.invalid
              << " sem=" << st2.semantic << " epi=" << st2.episodic
              << " proc=" << st2.procedural << std::endl;
    check(st2.invalid == 1, "失效记录数 == 1（被 supersede 的那条）",
          "实际 " + std::to_string(st2.invalid));
    check(st2.total == 4, "总记录数 == 4（4 写入 +1 supersede −1 remove）",
          "实际 " + std::to_string(st2.total));

    std::cout << "\n=== 结果: PASS " << g_pass << " / FAIL " << g_fail << " ===" << std::endl;
    return g_fail == 0 ? 0 : 1;
}
