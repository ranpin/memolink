#!/usr/bin/env python3
"""Q4 提取阶段评测：本地 4B 受约束 JSON 提取 → 测查全 + 编造率（LongMemEval has_answer 金标）。

设计依据 docs/memory_v2_design.md §6/§12：
- 提取 = Mem0 式，把对话经本地 LLM 抽成受约束 JSON 数组（text/type/owner/scope），非自由文本。
- 查全（recall）= 抽取的事实集是否覆盖金标 answer（answer 的 content token 被某条事实覆盖的比例）。
- 编造率（fabrication）= 抽取事实里「词汇不落地」的比例——事实的 content token 有多少出现在源对话里；
  提取本应从对话里抄，落地率低 = 引入了对话里没有的内容 = 疑似编造（P2 已实测 4B 开放式生成会编造）。
- 词汇匹配是**代理**：会漏判改写、丢失否定。故对低落地率事实打印原文供人工核验，不静默下结论。

用法:
  python3 extract_eval.py <longmemeval_oracle.json> --single <question_id>   # 冒烟：单题看输出+耗时
  python3 extract_eval.py <longmemeval_oracle.json> --n-per-type 5           # 分层子集评测
"""
import argparse
import json
import re
import sys
import time
import urllib.request
from collections import defaultdict

LLM_URL = "http://127.0.0.1:8099/v1/chat/completions"
MODEL = "qwen3-4b"

STOPWORDS = set("""the a an and or but if then than that this these those is are was were be been
being i you he she it we they me him her us them my your his its our their what which who whom when
where why how do does did done have has had having will would can could should may might must shall
of in on at to for with about against between through during before after above below up down out
off over under again further once here there all any both each few more most other some such no nor
not only own same so too very s t just don now by the way bye hi hello thanks thank ok okay yeah
really also get got going wanna gonna something anything nothing""".split())

EXTRACT_PROMPT = """/no_think
You are the memory-extraction module of an on-device agent. Extract persistent, reusable facts from the conversation.

Strict rules:
- Output ONLY a JSON array, no prose, no code fences.
- Each element: {{"text": "<one self-contained fact>", "type": "semantic|episodic|procedural", "owner": "user|assistant", "scope": "private|shared"}}
- type: semantic = facts/preferences/attributes; episodic = past events (keep any date/time); procedural = how-to / skills / routines.
- Extract ONLY what is explicitly stated. NEVER invent, infer, or add any detail not present in the conversation.
- Skip small talk, questions, and filler. If nothing is worth remembering, output [].

Conversation:
{conversation}

JSON array:"""


def call_llm(prompt, max_tokens=2048, temperature=0.0, timeout=240):
    body = json.dumps({
        "model": MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": temperature,
        "max_tokens": max_tokens,
    }).encode()
    req = urllib.request.Request(LLM_URL, data=body, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        resp = json.load(r)
    return resp["choices"][0]["message"]["content"]


def parse_facts(out):
    """从 LLM 输出里抠出第一个平衡的 JSON 数组；返回 (facts, parse_ok)。"""
    out = out.strip()
    start = out.find("[")
    if start == -1:
        return [], False
    depth = 0
    for i in range(start, len(out)):
        if out[i] == "[":
            depth += 1
        elif out[i] == "]":
            depth -= 1
            if depth == 0:
                try:
                    arr = json.loads(out[start:i + 1])
                    if isinstance(arr, list):
                        return [f for f in arr if isinstance(f, dict) and f.get("text")], True
                except json.JSONDecodeError:
                    pass
                break
    return [], False


# 数词归一：金标答案常用词形（four）而提取常写数字（4），不归一会造成假阴性
WORD_NUM = {
    "one": "1", "two": "2", "three": "3", "four": "4", "five": "5",
    "six": "6", "seven": "7", "eight": "8", "nine": "9", "ten": "10",
    "eleven": "11", "twelve": "12",
}


def tokens(text):
    # str()：LongMemEval 有 32 题 answer 是 int，不能直接 .lower()
    toks = [w for w in re.findall(r"[a-z0-9]+", str(text).lower())
            if w not in STOPWORDS and len(w) > 1]
    return [WORD_NUM.get(t, t) for t in toks]


def grounding_ratio(fact_text, source_tok_set):
    """事实的 content token 落在源对话里的比例。低 = 疑似编造。"""
    ft = tokens(fact_text)
    if not ft:
        return 1.0
    return len([t for t in ft if t in source_tok_set]) / len(ft)


def answer_coverage(answer, facts):
    """金标 answer 的 content token 被「事实集并集」覆盖的比例。
    用并集：答案可能散落在多条事实里；也容忍提取的蒸馏/改写（P2 已知改写会掉 token 重叠）。"""
    at = set(tokens(answer))
    if not at:
        return 1.0
    union = set()
    for f in facts:
        union |= set(tokens(f.get("text", "")))
    return len(at & union) / len(at)


def session_to_text(session):
    return "\n".join(f"{t.get('role','?')}: {t.get('content','')}" for t in session)


# llama-server 是 -c 4096 启动的（n_ctx=4096）。单段长对话（preference 题 ~6.4k tokens）
# 一次性喂会超上下文、报 HTTP 400（实测 5/5 preference 题因此 facts=0）。按 Mem0 滑窗思路
# （§6「最近 m 条消息」）把 session 切成字符预算内的窗口逐块提取——每块 prompt(~2k tok)
# + max_tokens(1280) 留足 4096 余量。
def chunk_session(session, max_chars=8000):
    chunks, cur, cur_len = [], [], 0
    for t in session:
        line = f"{t.get('role','?')}: {t.get('content','')}"
        if cur and cur_len + len(line) + 1 > max_chars:
            chunks.append("\n".join(cur))
            cur, cur_len = [], 0
        cur.append(line)
        cur_len += len(line) + 1
    if cur:
        chunks.append("\n".join(cur))
    return chunks


def extract_question(q, verbose=False):
    """对一题的所有 session 分块提取，返回 (facts, timings, parse_oks, source_toks)。"""
    sessions = q["haystack_sessions"]
    # 源对话全文 token 集（编造落地性对照用——事实落在全文任意处即算 grounded）
    source_text = "\n".join(session_to_text(s) for s in sessions)
    source_toks = set(tokens(source_text))

    all_facts, timings, parse_oks = [], [], []
    for si, sess in enumerate(sessions):
        for ci, chunk in enumerate(chunk_session(sess)):
            if not chunk.strip():
                continue
            t0 = time.time()
            try:
                out = call_llm(EXTRACT_PROMPT.format(conversation=chunk), max_tokens=1280)
            except Exception as e:  # noqa: BLE001
                timings.append(time.time() - t0)
                parse_oks.append(False)
                if verbose:
                    print(f"    [s{si} c{ci}] LLM 调用失败: {e}")
                continue
            dt = time.time() - t0
            timings.append(dt)
            facts, ok = parse_facts(out)
            parse_oks.append(ok)
            for f in facts:
                f["_session"] = si
                f["_grounding"] = grounding_ratio(f.get("text", ""), source_toks)
            all_facts.extend(facts)
            if verbose:
                print(f"    [s{si} c{ci}] {dt:.1f}s  parse_ok={ok}  抽出 {len(facts)} 条")
                if not ok:
                    print(f"      原始输出: {out[:300]}")
    return all_facts, timings, parse_oks, source_toks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data")
    ap.add_argument("--single", help="只跑这个 question_id（冒烟）")
    ap.add_argument("--n-per-type", type=int, default=0, help="每题型分层抽 N 题")
    ap.add_argument("--ground-thresh", type=float, default=0.7, help="落地率低于此判疑似编造")
    ap.add_argument("--recall-thresh", type=float, default=0.6, help="答案覆盖高于此算查全命中")
    args = ap.parse_args()

    with open(args.data, encoding="utf-8") as f:
        data = json.load(f)

    # 选子集
    if args.single:
        subset = [q for q in data if q["question_id"] == args.single]
        if not subset:
            print(f"找不到 question_id={args.single}", file=sys.stderr)
            return 2
    elif args.n_per_type > 0:
        by_type = defaultdict(list)
        for q in data:
            by_type[q["question_type"]].append(q)
        subset = []
        for qt in sorted(by_type):
            subset.extend(by_type[qt][:args.n_per_type])   # 取前 N（确定性，可复现）
    else:
        subset = data

    verbose = bool(args.single)
    print(f"=== Q4 提取评测 ===  子集 {len(subset)} 题  model={MODEL}")
    if args.n_per_type:
        tc = defaultdict(int)
        for q in subset:
            tc[q["question_type"]] += 1
        print(f"  分层: {dict(tc)}")
    print()

    tot_time = 0.0
    tot_facts = 0
    tot_parse_ok = 0
    tot_parse_calls = 0
    recall_hits = 0
    fab_facts = 0
    low_ground_samples = []
    low_cov_samples = []
    per_type = defaultdict(lambda: {"n": 0, "recall": 0})

    for qi, q in enumerate(subset):
        qid = q["question_id"]
        qtype = q["question_type"]
        answer = q["answer"]
        if verbose:
            print(f"[{qi+1}/{len(subset)}] {qid} ({qtype})")
            print(f"  Q: {q['question']}")
            print(f"  金标 answer: {answer}")

        facts, timings, parse_oks, _ = extract_question(q, verbose=verbose)
        qtime = sum(timings)
        tot_time += qtime
        tot_facts += len(facts)
        tot_parse_ok += sum(1 for o in parse_oks if o)
        tot_parse_calls += len(parse_oks)

        cov = answer_coverage(answer, facts)
        hit = cov >= args.recall_thresh
        if hit:
            recall_hits += 1
            per_type[qtype]["recall"] += 1
        per_type[qtype]["n"] += 1
        if not hit and len(low_cov_samples) < 15:
            low_cov_samples.append((qid, qtype, str(answer),
                                    [f.get("text", "")[:90] for f in facts]))

        qfab = 0
        for f in facts:
            if f["_grounding"] < args.ground_thresh:
                qfab += 1
                if len(low_ground_samples) < 25:
                    low_ground_samples.append((qid, round(f["_grounding"], 2), f.get("text", "")[:120]))
        fab_facts += qfab

        print(f"  [{qi+1}/{len(subset)}] {qtype[:22]:22s} facts={len(facts):2d} t={qtime:5.1f}s "
              f"cov={cov:.2f}{'✓' if hit else '✗'} fab={qfab}", flush=True)
        if verbose:
            for f in facts:
                print(f"    - [{f.get('type','?')}/{f.get('owner','?')}] g={f['_grounding']:.2f}  {f.get('text','')[:100]}")
            print()

    # ---- 汇总 ----
    n = len(subset)
    print("=== 汇总 ===")
    print(f"  题数: {n}   总提取耗时: {tot_time:.1f}s   单题均耗时: {tot_time/max(n,1):.1f}s")
    print(f"  JSON 解析成功率: {tot_parse_ok}/{tot_parse_calls} session 调用")
    print(f"  抽出事实总数: {tot_facts}   单题均: {tot_facts/max(n,1):.1f}")
    print(f"  答案查全（覆盖≥{args.recall_thresh}）: {recall_hits}/{n} = {100*recall_hits/max(n,1):.1f}%")
    print(f"  疑似编造（落地率<{args.ground_thresh}）: {fab_facts}/{tot_facts} = {100*fab_facts/max(tot_facts,1):.1f}%")
    print("\n  分题型查全:")
    for qt in sorted(per_type):
        d = per_type[qt]
        print(f"    {qt:28s} {d['recall']}/{d['n']}")
    if low_ground_samples:
        print(f"\n  低落地率事实样本（供人工核验是否真编造，最多 25 条）:")
        for qid, g, txt in low_ground_samples:
            print(f"    g={g:.2f} [{qid[:16]}] {txt}")
    if low_cov_samples:
        print(f"\n  低查全题样本（金标答案 vs 抽取事实，判断真没抽到还是度量假象，最多 15 题）:")
        for qid, qt, ans, ftxts in low_cov_samples:
            print(f"    [{qt[:18]}] {qid[:16]} answer={ans[:70]}")
            for ft in ftxts[:6]:
                print(f"        · {ft}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
