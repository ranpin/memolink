#include "memory/core/MemoryResolver.h"
#include "infra/llm/QwenAPI.h"
#include <json/json.h>
#include <algorithm>
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------
// 内部工具：从 LLM 自由输出里抠出第一个含 "action" 字段的平衡 JSON 块。
// qwen3 可能先输出一段 thinking 再给答案，thinking 里也可能有花括号，
// 所以逐块尝试解析、只认带 action 的那块，而不是简单取首尾花括号。
// ---------------------------------------------------------------------------
namespace {

bool extractActionJson(const std::string& text, std::string& action, int& targetId) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '{') continue;
        int depth = 0;
        for (size_t j = i; j < text.size(); ++j) {
            if (text[j] == '{') {
                ++depth;
            } else if (text[j] == '}') {
                --depth;
                if (depth == 0) {
                    const std::string block = text.substr(i, j - i + 1);
                    Json::Value root;
                    Json::CharReaderBuilder rb;
                    std::string errs;
                    std::istringstream is(block);
                    if (Json::parseFromStream(rb, is, &root, &errs) &&
                        root.isObject() && root.isMember("action")) {
                        action = root["action"].asString();
                        targetId = root.get("target_id", 0).asInt();
                        return true;
                    }
                    break;   // 此块不含 action，换下一个 '{' 再试
                }
            }
        }
    }
    return false;
}

// System-2 用的模型名：刻意不含 "omni"（QwenAPI 见到 omni 会切流式解析）。
// llama-server 实际忽略该名、只用已加载的模型，这里只为避开流式分支。
constexpr const char* kSystem2Model = "qwen3-4b";

}  // namespace

// ---------------------------------------------------------------------------
MemoryResolver::MemoryResolver(MemoryStore& store, QwenAPI* llm)
    : store_(store), llm_(llm) {}

// ---------------------------------------------------------------------------
// Q9：authority 优先级闸门（docs/q9_multimodal_plan.md §1.4）
// 此前 authority 只用于治理过滤（MemoryStore.cpp 的 `AND authority = ?`），从不参与
// 消解；多模态引入 observed 来源后，让它第一次参与决策。纯函数 + public static，
// 便于无 LLM 时离线单测（System-2 回退 CREATE 时闸门逻辑仍可独立验证）。
// ---------------------------------------------------------------------------
int MemoryResolver::authorityRank(const std::string& authority) {
    if (authority == "observed") return 2;
    if (authority == "inferred") return 0;
    return 1;   // asserted 及任何未知值一律按 asserted 处理
}

void MemoryResolver::applyAuthorityGate(ResolveDecision& d, const Memory& target,
                                        const std::string& candAuthority) {
    if (d.targetId == 0 || d.targetId != target.id) return;   // 无目标/目标不匹配，无需裁决
    const int rc = authorityRank(candAuthority);
    const int rt = authorityRank(target.authority);
    if (rc == rt) return;   // 同 authority → 现有五操作行为逐条不变（M3 硬闸门保证）

    if (rc > rt) {
        // 高可信度来源胜出：UPDATE 升级为 SUPERSEDE（观察取代口述，建版本链）。
        // SUPERSEDE 本就正确、REINFORCE 是同一事实，均不动。
        if (d.action == MemoryAction::Updated) {
            d.action = MemoryAction::Superseded;
            d.authorityPromoted = true;
            d.reason += " | authority 闸门: " + candAuthority + " > " + target.authority
                        + " → UPDATE 升级为 SUPERSEDE";
        }
    } else {
        // 低可信度来源不得静默改写高可信度观察：SUPERSEDE/UPDATE 降级为 CREATE
        // （入库但不建取代关系，两条并存——用户 2026-10-25 拍板的「拦下」语义）。
        if (d.action == MemoryAction::Superseded || d.action == MemoryAction::Updated) {
            d.blockedByAuthority = true;
            d.blockedTargetId = d.targetId;
            d.targetId = 0;
            d.action = MemoryAction::Created;
            d.reason += " | authority 闸门拦下: " + candAuthority + " < " + target.authority
                        + " → 不覆盖，改为并存 CREATE";
        }
    }
}

ResolveDecision MemoryResolver::resolve(const std::string& candidate, MemoryType type,
                                        const std::string& authority,
                                        const std::string& sourceImage) {
    ResolveDecision d;
    if (candidate.empty()) {
        d.action = MemoryAction::Failed;
        d.reason = "empty candidate";
        return d;
    }

    const std::vector<float> emb = store_.embedText(candidate);
    if (emb.empty()) {
        d.action = MemoryAction::Failed;
        d.reason = "embedding failed";
        return d;
    }

    // 与同类型、当前有效的记忆比对（不含历史版本；消解只关心现行记忆）。
    // Q9：sourceImage 非空时**过滤掉同一张图批次的记忆**——同一张图拆出的多条事实是
    // 同一时刻的互补观察，不应互相取代（q9_multimodal_plan.md §8.6 问题 1：实测
    // 「车内安装了儿童安全座椅」被同图的「座椅上贴有TAKTRA标签」SUPERSEDE 作废）。
    // 多取一些再过滤，避免过滤后丢掉真正该比对的跨来源记忆。
    const int searchK = sourceImage.empty() ? kSystem2TopK : kSystem2TopK * 4;
    std::vector<Memory> matches =
        store_.searchByEmbedding(emb, searchK, type, /*includeHistory=*/false);
    if (!sourceImage.empty()) {
        matches.erase(std::remove_if(matches.begin(), matches.end(),
                                     [&](const Memory& m) { return m.sourceImage == sourceImage; }),
                      matches.end());
        if (matches.size() > static_cast<size_t>(kSystem2TopK))
            matches.resize(kSystem2TopK);
    }

    if (matches.empty()) {
        d.action = MemoryAction::Created;
        d.similarity = 0.f;
        d.reason = sourceImage.empty()
            ? "System-1: 空库/无同类型记忆 → CREATE"
            : "System-1: 仅同图批次记忆、无跨来源冲突 → CREATE";
        return d;
    }

    const float top = matches[0].score;
    d.similarity = top;

    // ---- System-1：确定性快路径（不调模型）----
    if (top < kThetaNew) {
        d.action = MemoryAction::Created;
        d.reason = "System-1: score < θ_new → CREATE";
        return d;
    }
    if (top >= kThetaDup) {
        d.action = MemoryAction::Reinforced;
        d.targetId = matches[0].id;
        d.reason = "System-1: score ≥ θ_dup → REINFORCE（近逐字）";
        d.preGateAction = d.action;
        applyAuthorityGate(d, matches[0], authority);   // REINFORCE 是同一事实，闸门不改它
        return d;
    }

    // ---- System-2：模糊地带交给本地 LLM ----
    d = runSystem2(candidate, matches);
    d.preGateAction = d.action;   // 记录过闸门前 System-2 的原始判定（M2 before/after 对照）

    // ---- Q9：authority 闸门（只在候选与目标 authority 不同时介入）----
    if (d.targetId != 0) {
        for (const auto& m : matches) {
            if (m.id == d.targetId) { applyAuthorityGate(d, m, authority); break; }
        }
    }
    return d;
}

ResolveDecision MemoryResolver::runSystem2(const std::string& candidate,
                                           const std::vector<Memory>& matches) {
    ResolveDecision d;
    d.similarity = matches.empty() ? 0.f : matches[0].score;

    if (!llm_) {
        d.action = MemoryAction::Created;
        d.usedLlm = false;
        d.reason = "System-2 跳过（无 LLM）→ 回退 CREATE";
        return d;
    }

    std::ostringstream p;
    p << "/no_think\n"
      << "你是端侧记忆系统的消解器。判断「候选记忆」相对「已有相似记忆」应执行哪种操作，只能四选一：\n"
      << "- UPDATE：候选**补充了新的具体细节**（如新的数值/地点/时间/子类/步骤），信息量更大，且**没有改变**已有记忆里的任何值。\n"
      << "- SUPERSEDE：候选**改变或推翻了**已有记忆里的某个值（如颜色/地址/日期/数值/状态/路线换成新的），新旧不能同时成立。\n"
      << "- REINFORCE：候选与已有记忆**说的是同一件事**，只是换种说法，或是信息更少的版本；**没有任何新的具体细节**。\n"
      << "- NOOP：候选是寒暄/闲聊/提问/确认，不构成值得长期保存的事实。\n\n"
      << "候选记忆：「" << candidate << "」\n\n"
      << "已有相似记忆（按相似度降序）：\n";
    for (const auto& m : matches) {
        p << "[id=" << m.id << "]「" << m.content << "」\n";
    }
    p << "\n只输出一行 JSON，禁止任何解释或思考过程：\n"
      << "{\"action\":\"UPDATE|SUPERSEDE|REINFORCE|NOOP\",\"target_id\":整数}\n"
      << "target_id 为被操作的已有记忆 id；NOOP 或无合适目标时填 0。";

    QwenChatCompletionRequest req;
    req.model = kSystem2Model;
    req.messages.push_back(QwenMessage{"user", p.str(), {}});
    req.temperature = 0.0f;
    req.top_p = 1.0f;
    req.max_tokens = 128;

    QwenChatCompletionResponse resp;
    try {
        resp = llm_->chat_completion(req);
    } catch (...) {
        d.action = MemoryAction::Created;
        d.usedLlm = false;
        d.reason = "System-2 LLM 抛异常 → 回退 CREATE";
        return d;
    }
    d.usedLlm = true;

    const std::string& out = resp.content;
    if (out.rfind("Error:", 0) == 0) {
        d.action = MemoryAction::Created;
        d.reason = "System-2 LLM 调用失败 → 回退 CREATE; " + out;
        return d;
    }

    std::string act;
    int targetId = 0;
    if (!extractActionJson(out, act, targetId)) {
        d.action = MemoryAction::Created;
        d.reason = "System-2 输出解析失败 → 回退 CREATE; raw=" + out;
        return d;
    }

    // target_id 合法性校验：必须落在喂进去的候选集里，否则退回最相似那条
    int validTarget = 0;
    for (const auto& m : matches) {
        if (m.id == targetId) { validTarget = targetId; break; }
    }
    if (validTarget == 0 && !matches.empty()) validTarget = matches[0].id;

    d.reason = "System-2: " + out;
    if (act == "UPDATE")         { d.action = MemoryAction::Updated;    d.targetId = validTarget; }
    else if (act == "SUPERSEDE") { d.action = MemoryAction::Superseded; d.targetId = validTarget; }
    else if (act == "REINFORCE") { d.action = MemoryAction::Reinforced; d.targetId = validTarget; }
    else if (act == "NOOP")      { d.action = MemoryAction::Noop;       d.targetId = 0; }
    else {
        d.action = MemoryAction::Created;
        d.reason = "System-2 未知动作 '" + act + "' → 回退 CREATE";
    }
    return d;
}

IngestResult MemoryResolver::ingest(const std::string& candidate, MemoryType type,
                                    const std::string& owner, const std::string& scope,
                                    const std::string& authority,
                                    const std::string& sourceSession, int sourceTurn,
                                    const std::string& sourceImage) {
    const ResolveDecision d = resolve(candidate, type, authority, sourceImage);

    IngestResult r;
    r.similarity = d.similarity;
    r.usedLlm = d.usedLlm;
    r.reason = d.reason;
    r.blockedByAuthority = d.blockedByAuthority;
    r.authorityPromoted = d.authorityPromoted;
    r.preGateAction = d.preGateAction;

    switch (d.action) {
        case MemoryAction::Created: {
            // 直接走 MemoryStore 的 CREATE（保留 owner/scope/authority/来源）。
            // 被 authority 闸门拦下的候选也走这里：入库但不建取代关系，两条并存。
            IngestResult created = store_.ingestText(candidate, type, owner, scope,
                                                     authority, sourceSession, sourceTurn,
                                                     /*validFrom=*/"", sourceImage);
            created.similarity = d.similarity;
            created.usedLlm = d.usedLlm;
            created.blockedByAuthority = d.blockedByAuthority;
            created.preGateAction = d.preGateAction;
            if (created.action == MemoryAction::Created) created.reason = d.reason;
            return created;
        }
        case MemoryAction::Updated: {
            r.action = store_.update(d.targetId, candidate) ? MemoryAction::Updated
                                                            : MemoryAction::Failed;
            r.id = d.targetId;
            return r;
        }
        case MemoryAction::Reinforced: {
            r.action = store_.reinforce(d.targetId) ? MemoryAction::Reinforced
                                                    : MemoryAction::Failed;
            r.id = d.targetId;
            return r;
        }
        case MemoryAction::Superseded: {
            int newId = 0;
            // Q9：新记录带**候选**的 authority + source_image。observed 取代 asserted 时
            // 必须写 observed，不能继承旧记录的 asserted（否则 authority 优先级白做）。
            const bool ok = store_.supersede(d.targetId, candidate, &newId,
                                             authority, sourceImage);
            r.action = ok ? MemoryAction::Superseded : MemoryAction::Failed;
            r.id = newId;
            return r;
        }
        case MemoryAction::Noop:
            r.action = MemoryAction::Noop;
            r.id = 0;
            return r;
        default:
            r.action = MemoryAction::Failed;
            return r;
    }
}
