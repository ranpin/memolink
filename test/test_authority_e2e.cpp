#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <sqlite3.h>
#include <json/json.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "memory/core/MemoryStore.h"
#include "memory/core/MemoryResolver.h"
#include "infra/llm/QwenAPI.h"

// ============================================================================
// Q9 · M2 完整测量：authority 感知消解在真实「System-2 → 闸门 → 落库」链路里的正确率。
// ----------------------------------------------------------------------------
// 与 test_multimodal_authority 的分工：那个测闸门**纯函数真值表**（离线确定性）；
// 本测测端到端链路，场景集在 bench/eval/authority_scenarios.json（措辞复用 Q2 已验证
// 能稳定触发 SUPERSEDE/UPDATE 的 pair，避免自编措辞让 System-2 判不出冲突）。
//
// 断言策略（对 LLM 非确定性稳健）：不假设 System-2 一定产出某动作，而是
//   **给定它实际产出的 preGateAction，验闸门是否做出正确的 authority 感知结果**。
//   preGateAction ∉ {SUPERSEDE, UPDATE} 的场景视为「未触达闸门」，不计入闸门样本，
//   单独统计——这样 System-2 的判定质量与闸门的判定质量被分开归因。
//
// 用法: test_authority_e2e [scenarios.json] [db路径]
// 前置：config.ini [edge] 配好模型/分词器；[api] local_url 指向本地 llama-server
//       （:8099 qwen3-4b）。LLM 不可用则模糊地带回退 CREATE、触达不了闸门。
// ============================================================================

static int g_pass = 0, g_fail = 0;
static void check(bool cond, const std::string& name, const std::string& detail = "") {
    if (cond) { ++g_pass; std::cout << "    [PASS] " << name; }
    else { ++g_fail; std::cout << "    [FAIL] " << name; }
    if (!detail.empty()) std::cout << " — " << detail;
    std::cout << std::endl;
}

static const char* actName(MemoryAction a) {
    switch (a) {
        case MemoryAction::Created:    return "CREATE";
        case MemoryAction::Updated:    return "UPDATE";
        case MemoryAction::Reinforced: return "REINFORCE";
        case MemoryAction::Superseded: return "SUPERSEDE";
        case MemoryAction::Noop:       return "NOOP";
        case MemoryAction::Removed:    return "REMOVED";
        default:                       return "FAILED";
    }
}

static void clearMemory(StorageManager& storage) {
    sqlite3* db = storage.getDatabase();
    if (db) sqlite3_exec(db, "DELETE FROM memory;", nullptr, nullptr, nullptr);
}

int main(int argc, char** argv) {
    const std::string scenPath = argc > 1 ? argv[1] : "../bench/eval/authority_scenarios.json";
    const std::string dbPath   = argc > 2 ? argv[2] : "../database/authority_e2e.db";

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
    if (!scenarios.isArray() || scenarios.empty()) { std::cerr << "场景集为空\n"; return 2; }

    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }
    MemoryStore store(storage);
    if (!store.ready()) { std::cerr << "store 未就绪（[edge] 模型没配？）\n"; return 1; }

    QwenAPI llm;
    MemoryResolver resolver(store, &llm);

    std::cout << "=== Q9 · M2 完整测量：authority 感知消解端到端 ===\n"
              << "  场景数=" << scenarios.size() << "  embed=" << store.embedModelTag()
              << "  θ_new=" << MemoryResolver::kThetaNew
              << "  θ_dup=" << MemoryResolver::kThetaDup << "\n\n";

    // 分组统计：触达闸门数 / 闸门判定正确数
    std::map<std::string, int> grpTotal, grpReached, grpCorrect;
    int promotionFired = 0, promotionExpected = 0;

    for (const auto& s : scenarios) {
        const std::string id   = s["id"].asString();
        const std::string grp  = s.get("group", "?").asString();
        const std::string expectGate = s["expect_gate"].asString();
        const std::string seedAuth = s["seed_authority"].asString();
        const std::string candAuth = s["cand_authority"].asString();
        MemoryType type = MemoryType::Semantic;
        memoryTypeFromName(s.get("type", "semantic").asString(), type);

        grpTotal[grp]++;
        std::cout << "── " << id << "  [" << grp << "]  seed=" << seedAuth
                  << " cand=" << candAuth << " 期望闸门=" << expectGate << "\n";

        clearMemory(storage);
        const auto seedR = store.ingestText(s["seed"].asString(), type, "张三", "private", seedAuth);
        if (seedR.action != MemoryAction::Created) {
            std::cout << "    [SKIP] seed 写入失败\n\n"; continue;
        }
        const int seedId = seedR.id;

        const IngestResult r = resolver.ingest(s["candidate"].asString(), type,
                                               "张三", "private", candAuth);

        const int candRank = MemoryResolver::authorityRank(candAuth);
        const int seedRank = MemoryResolver::authorityRank(seedAuth);
        const bool authDiffer = (candRank != seedRank);
        const bool preGateConflict = (r.preGateAction == MemoryAction::Superseded ||
                                      r.preGateAction == MemoryAction::Updated);
        const bool gateReached = authDiffer && preGateConflict;

        std::cout << "    sim=" << r.similarity
                  << "  preGate=" << actName(r.preGateAction)
                  << "  final=" << actName(r.action)
                  << (r.blockedByAuthority ? "  [被拦下]" : "")
                  << (r.authorityPromoted ? "  [升级]" : "")
                  << (r.usedLlm ? "  [S2]" : "  [S1]") << "\n";

        if (!gateReached) {
            std::cout << "    [INFO] 未触达闸门（preGate=" << actName(r.preGateAction)
                      << (authDiffer ? "，非冲突动作" : "，authority 相同") << "）\n";
            // 同 authority 对照：即便不触达闸门也要验「闸门没乱动」
            if (expectGate == "none") {
                grpReached[grp]++;
                bool ok = (r.action == r.preGateAction);
                check(ok, "对照：final == preGate（闸门未介入）",
                      std::string("preGate=") + actName(r.preGateAction) + " final=" + actName(r.action));
                if (r.action == MemoryAction::Superseded) {
                    const Memory nm = store.get(r.id);
                    check(nm.authority == candAuth, "对照：SUPERSEDE 新记录 authority 正确");
                    ok = ok && (nm.authority == candAuth);
                }
                if (r.action == MemoryAction::Updated) {
                    const Memory nm = store.get(r.id);
                    check(nm.authority == seedAuth && nm.valid,
                          "对照：UPDATE 原地改写、authority 不变、记录仍有效");
                    ok = ok && (nm.authority == seedAuth);
                }
                if (ok) grpCorrect[grp]++;
            }
            std::cout << "\n";
            continue;
        }

        grpReached[grp]++;
        bool ok = true;

        if (expectGate == "win") {
            const Memory newM = store.get(r.id);
            const Memory oldM = store.get(seedId);
            if (r.preGateAction == MemoryAction::Updated) {
                promotionExpected++;
                if (r.authorityPromoted) promotionFired++;
                check(r.authorityPromoted, "胜出(升级)：UPDATE 被升级为 SUPERSEDE");
            }
            check(r.action == MemoryAction::Superseded && !r.blockedByAuthority,
                  "胜出：final=SUPERSEDE 且未被拦", std::string("final=") + actName(r.action));
            check(newM.authority == candAuth, "胜出：新记录 authority=" + candAuth,
                  "实际「" + newM.authority + "」");
            check(!oldM.valid && oldM.supersededBy == r.id, "胜出：旧记录失效 + superseded_by 指向新记录");
            check(newM.supersedes == seedId, "胜出：新记录 supersedes 指向旧记录");
            ok = (r.action == MemoryAction::Superseded && newM.authority == candAuth &&
                  !oldM.valid && oldM.supersededBy == r.id && newM.supersedes == seedId);
        } else if (expectGate == "block") {
            const Memory newM = store.get(r.id);
            const Memory oldM = store.get(seedId);
            check(r.action == MemoryAction::Created && r.blockedByAuthority,
                  "拦下：final=CREATE 且 blockedByAuthority=true",
                  std::string("final=") + actName(r.action) + (r.blockedByAuthority ? " blocked" : " NOT-blocked"));
            check(oldM.valid && oldM.supersededBy == 0,
                  "拦下：高可信度旧记录仍有效、未被取代", "valid=" + std::to_string(oldM.valid));
            check(newM.valid && newM.authority == candAuth,
                  "拦下：低可信度候选作为独立记忆并存", "authority=" + newM.authority);
            check(newM.supersedes == 0, "拦下：并存记忆不建取代关系");
            ok = (r.action == MemoryAction::Created && r.blockedByAuthority &&
                  oldM.valid && oldM.supersededBy == 0 && newM.supersedes == 0);
        } else {
            std::cout << "    [WARN] expect_gate=" << expectGate << " 但触达了闸门——场景标注可能有误\n";
            ok = false;
        }

        if (ok) grpCorrect[grp]++;
        std::cout << "\n";
    }

    // ---- 汇总 ----
    std::cout << "=== M2 分组汇总（触达闸门 / 判定正确）===\n";
    int totReached = 0, totCorrect = 0, totAll = 0;
    for (const auto& kv : grpTotal) {
        const std::string& g = kv.first;
        std::cout << "  " << g;
        for (size_t i = g.size(); i < 10; ++i) std::cout << " ";
        std::cout << " 场景 " << kv.second
                  << "  触达闸门 " << grpReached[g]
                  << "  正确 " << grpCorrect[g] << "/" << grpReached[g] << "\n";
        totAll += kv.second; totReached += grpReached[g]; totCorrect += grpCorrect[g];
    }
    std::cout << "\n  合计: 场景 " << totAll << "，触达闸门 " << totReached
              << "，闸门判定正确 " << totCorrect << "/" << totReached << "\n";
    std::cout << "  升级路径（UPDATE→SUPERSEDE）端到端触发: " << promotionFired
              << "/" << promotionExpected << "\n";
    std::cout << "  单元断言: " << g_pass << " PASS / " << g_fail << " FAIL\n";
    if (totReached == 0)
        std::cout << "  ⚠️ 没有场景触达闸门——检查 LLM 是否可用、场景相似度是否落在 System-2 区间\n";

    return (g_fail == 0 && totReached > 0) ? 0 : 1;
}
