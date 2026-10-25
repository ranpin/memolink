#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <sqlite3.h>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include "memory/core/MemoryStore.h"
#include "memory/core/MemoryResolver.h"
#include "infra/llm/QwenAPI.h"

// ============================================================================
// Q9 · M2 端到端：authority 感知消解在真实 resolve()+ingest() 链路里是否正确触发。
// ----------------------------------------------------------------------------
// 与 test_multimodal_authority 的分工：那个测的是闸门**纯函数逻辑**（B 部分）与
// 存储路径（C 部分）；本测测的是「**System-2（本地 LLM）产出 SUPERSEDE/UPDATE →
// 闸门 → 落库**」的完整链路——这才是 M2 的端到端判据（docs/q9_multimodal_plan.md §2）。
//
// 断言策略（对 LLM 非确定性稳健）：不假设 System-2 一定产出某个动作，而是
//   **给定它实际产出的 preGateAction，验证闸门做出了正确的 authority 感知结果**。
//   同时报告有多少场景真正触达了闸门（preGateAction ∈ {SUPERSEDE, UPDATE} 且
//   候选/目标 authority 不同）——没触达的场景不算闸门测试样本。
//
// 用法: test_authority_e2e [db路径]
// 前置：config.ini [edge] 配好模型/分词器；[api] local_url 指向本地 llama-server
//       （System-2 用，:8099 的 qwen3-4b）。LLM 不可用则无法触达闸门，测试跳过。
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

struct Scenario {
    std::string name;
    std::string seedText;
    std::string seedAuthority;
    std::string candText;
    std::string candAuthority;
    std::string desc;
};

int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "../database/authority_e2e.db";

    if (std::filesystem::exists(dbPath)) std::filesystem::remove(dbPath);
    if (!InitDB::initializeDatabase(dbPath)) { std::cerr << "DB init failed\n"; return 1; }
    StorageManager storage(dbPath);
    if (!storage.initialize()) { std::cerr << "storage init failed\n"; return 1; }
    MemoryStore store(storage);
    if (!store.ready()) { std::cerr << "store 未就绪（[edge] 模型没配？）\n"; return 1; }

    QwenAPI llm;                       // config.ini [api] local_url → 本地 llama-server
    MemoryResolver resolver(store, &llm);

    std::cout << "=== Q9 · M2 端到端：authority 感知消解（System-2 → 闸门 → 落库）===\n"
              << "  embed=" << store.embedModelTag()
              << "  θ_new=" << MemoryResolver::kThetaNew
              << "  θ_dup=" << MemoryResolver::kThetaDup << "\n\n";

    // 场景集：覆盖 胜出 / 拦下 / 升级 / 同 authority 对照 四类。
    // 文本取自座舱 demo 已验证能可靠触发 SUPERSEDE 的单值矛盾（西湖区→滨江区 sim≈0.755）。
    const std::vector<Scenario> scenarios = {
        {"S1 胜出", "用户住在杭州西湖区，家庭地址已存入导航常用地点。", "asserted",
                     "用户搬家，家庭地址从西湖区改为滨江区", "observed",
                     "observed 候选 vs asserted 已有 → 应 SUPERSEDE，新记录带 observed"},
        {"S2 胜出(停车)", "车辆停在 B1 层", "asserted",
                     "摄像头观察到车辆实际停在 B2 层", "observed",
                     "observed 候选 vs asserted 已有 → 应 SUPERSEDE"},
        {"S3 拦下", "摄像头观察到车辆停在 B2 层靠电梯口的位置", "observed",
                     "驾驶员说他记得车辆停在 B1 层", "asserted",
                     "asserted 候选 vs observed 已有 → 应被拦下，两条并存"},
        {"S4 拦下(地址)", "摄像头观察到车辆登记地址为杭州滨江区", "observed",
                     "用户说家庭地址其实在杭州西湖区", "asserted",
                     "asserted 候选 vs observed 已有 → 应被拦下"},
        {"S5 升级", "车辆是一辆 SUV", "asserted",
                     "摄像头观察到车辆是一辆红色的新能源 SUV", "observed",
                     "observed 候选补充细节（UPDATE）→ 应升级为 SUPERSEDE"},
        {"S6 对照(同authority)", "用户住在杭州西湖区，家庭地址已存入导航常用地点。", "asserted",
                     "用户搬家，家庭地址从西湖区改为滨江区", "asserted",
                     "同 asserted → 闸门不介入，正常 SUPERSEDE（M3 端到端对照）"},
    };

    int reachedGate = 0, gateCorrect = 0;

    for (const auto& sc : scenarios) {
        std::cout << "── " << sc.name << "：" << sc.desc << "\n";
        clearMemory(storage);

        // seed（直接 CREATE，带指定 authority）
        const auto seedR = store.ingestText(sc.seedText, MemoryType::Semantic,
                                            "张三", "private", sc.seedAuthority);
        if (seedR.action != MemoryAction::Created) {
            std::cout << "    [SKIP] seed 写入失败\n\n"; continue;
        }
        const int seedId = seedR.id;

        // 消解 + 落库（候选带 candAuthority）
        const IngestResult r = resolver.ingest(sc.candText, MemoryType::Semantic,
                                               "张三", "private", sc.candAuthority);

        const int candRank = MemoryResolver::authorityRank(sc.candAuthority);
        const int seedRank = MemoryResolver::authorityRank(sc.seedAuthority);
        const bool authDiffer = (candRank != seedRank);
        const bool preGateConflict =
            (r.preGateAction == MemoryAction::Superseded || r.preGateAction == MemoryAction::Updated);
        const bool gateReached = authDiffer && preGateConflict;

        std::cout << "    sim=" << r.similarity
                  << "  preGate=" << actName(r.preGateAction)
                  << "  final=" << actName(r.action)
                  << (r.blockedByAuthority ? "  [被拦下]" : "")
                  << (r.authorityPromoted ? "  [升级]" : "")
                  << (r.usedLlm ? "  [S2]" : "  [S1]") << "\n";

        if (!gateReached) {
            std::cout << "    [INFO] 未触达闸门（preGate=" << actName(r.preGateAction)
                      << (authDiffer ? "" : "，且 authority 相同")
                      << "）——不计入闸门样本\n\n";
            // 同 authority 对照（S6）：即便不触达闸门，也应验证行为正常
            if (!authDiffer) {
                const Memory newM = store.get(r.id);
                check(r.action == r.preGateAction, "同 authority：final == preGate（闸门未介入）",
                      std::string("final=") + actName(r.action));
                if (r.action == MemoryAction::Superseded)
                    check(newM.authority == sc.candAuthority, "同 authority SUPERSEDE：新记录 authority 正确");
            }
            continue;
        }

        ++reachedGate;
        bool ok = true;

        if (candRank > seedRank) {
            // 胜出方向：应 SUPERSEDE，新记录带候选 authority，旧记录失效 + 版本链
            const Memory newM = store.get(r.id);
            const Memory oldM = store.get(seedId);
            check(r.action == MemoryAction::Superseded && !r.blockedByAuthority,
                  "胜出：final=SUPERSEDE 且未被拦", std::string("final=") + actName(r.action));
            check(newM.authority == sc.candAuthority,
                  "胜出：新记录 authority=" + sc.candAuthority, "实际「" + newM.authority + "」");
            check(!oldM.valid && oldM.supersededBy == r.id, "胜出：旧记录失效 + superseded_by 指向新记录");
            check(newM.supersedes == seedId, "胜出：新记录 supersedes 指向旧记录");
            if (r.preGateAction == MemoryAction::Updated)
                check(r.authorityPromoted, "胜出：UPDATE 被升级为 SUPERSEDE（authorityPromoted）");
            ok = (r.action == MemoryAction::Superseded && newM.authority == sc.candAuthority);
        } else {
            // 拦下方向：应降级为并存 CREATE，两条都有效，observed 仍是当前值
            const Memory newM = store.get(r.id);
            const Memory oldM = store.get(seedId);
            check(r.action == MemoryAction::Created && r.blockedByAuthority,
                  "拦下：final=CREATE 且 blockedByAuthority=true",
                  std::string("final=") + actName(r.action) + (r.blockedByAuthority ? " blocked" : " NOT-blocked"));
            check(oldM.valid && oldM.supersededBy == 0,
                  "拦下：高可信度旧记录仍有效、未被取代", "valid=" + std::to_string(oldM.valid));
            check(newM.valid && newM.authority == sc.candAuthority,
                  "拦下：低可信度候选作为独立记忆并存", "authority=" + newM.authority);
            check(newM.supersedes == 0, "拦下：并存记忆不建取代关系");
            ok = (r.action == MemoryAction::Created && r.blockedByAuthority && oldM.valid);
        }

        if (ok) ++gateCorrect;
        std::cout << "\n";
    }

    std::cout << "=== M2 端到端结果 ===\n";
    std::cout << "  触达闸门的场景: " << reachedGate << "/" << scenarios.size() << "\n";
    std::cout << "  闸门判定正确:   " << gateCorrect << "/" << reachedGate << "\n";
    std::cout << "  单元断言:       " << g_pass << " PASS / " << g_fail << " FAIL\n";
    if (reachedGate == 0)
        std::cout << "  ⚠️ 没有场景触达闸门——检查 LLM 是否可用、场景相似度是否落在 System-2 区间\n";
    return (g_fail == 0 && reachedGate > 0) ? 0 : 1;
}
