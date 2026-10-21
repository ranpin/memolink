#!/usr/bin/env python3
"""Q2 阈值标定探针：测每个消解场景的 top-1 相似度，按期望动作分组看分布。

为什么要这一步：三层消解的两个阈值（θ_new 判 CREATE、θ_dup 判 REINFORCE）
**必须从实测分布推导，不能拍脑袋**——否则五分类准确率无从解释，也无法判断
「System-1 覆盖率」是真高还是阈值放得太松。

关键设计点：消解比的是**记忆 vs 记忆**（候选事实 vs 已有记忆），两侧都是
passage，所以**都不加 BGE 的 query 指令前缀**（那个前缀只用于非对称检索）。

用法: python3 calibrate_thresholds.py <tokenizer.json> <model.onnx> <scenarios.json>
"""
import json
import sys
from collections import defaultdict

import numpy as np
import onnxruntime as ort
from tokenizers import Tokenizer


def make_embedder(tok_path, model_path, threads=4):
    tok = Tokenizer.from_file(tok_path)
    pad_id = tok.token_to_id("[PAD]")
    tok.enable_padding(direction="right", pad_id=pad_id, pad_token="[PAD]")
    tok.enable_truncation(max_length=128)

    so = ort.SessionOptions()
    so.intra_op_num_threads = threads
    so.inter_op_num_threads = 1
    so.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    sess = ort.InferenceSession(model_path, sess_options=so, providers=["CPUExecutionProvider"])
    in_names = [i.name for i in sess.get_inputs()]
    out_names = [o.name for o in sess.get_outputs()]

    def embed(texts):
        enc = tok.encode_batch(list(texts))
        feed = {}
        if "input_ids" in in_names:
            feed["input_ids"] = np.array([e.ids for e in enc], dtype=np.int64)
        if "attention_mask" in in_names:
            feed["attention_mask"] = np.array([e.attention_mask for e in enc], dtype=np.int64)
        if "token_type_ids" in in_names:
            feed["token_type_ids"] = np.array([e.type_ids for e in enc], dtype=np.int64)
        out = sess.run(None, feed)
        oi = out_names.index("sentence_embedding") if "sentence_embedding" in out_names else 0
        emb = np.asarray(out[oi], dtype=np.float32)
        return emb / (np.linalg.norm(emb, axis=1, keepdims=True) + 1e-9)

    def token_ids(text):
        """不分词成 subword 字符串，只取 id 集合——用于算新颖度。"""
        return [t for t in tok.encode(text).ids]

    return embed, token_ids


def main():
    if len(sys.argv) < 4:
        print("usage: calibrate_thresholds.py <tokenizer.json> <model.onnx> <scenarios.json>",
              file=sys.stderr)
        return 1
    tok_path, model_path, scen_path = sys.argv[1], sys.argv[2], sys.argv[3]
    embed, token_ids = make_embedder(tok_path, model_path)

    with open(scen_path, encoding="utf-8") as f:
        scenarios = json.load(f)["scenarios"]

    rows = []
    for s in scenarios:
        seeds, cand = s["seed"], s["candidate"]
        if not seeds:
            rows.append({"id": s["id"], "expected": s["expected"], "score": 0.0,
                         "top_idx": -1, "want_idx": s["expected_target"], "cand": cand,
                         "new_ratio": 1.0, "len_ratio": 0.0})
            continue
        vecs = embed(seeds + [cand])
        sims = vecs[:-1] @ vecs[-1]
        top_idx = int(np.argmax(sims))
        # 新颖度：候选相对匹配到的 seed 的新 token 占比（Mem0 的 InformationContent 判据）
        seed_toks = set(token_ids(seeds[top_idx]))
        cand_toks = token_ids(cand)
        new_ratio = len([t for t in cand_toks if t not in seed_toks]) / max(1, len(cand_toks))
        len_ratio = len(cand) / max(1, len(seeds[top_idx]))
        rows.append({"id": s["id"], "expected": s["expected"], "score": float(sims[top_idx]),
                     "top_idx": top_idx, "want_idx": s["expected_target"], "cand": cand,
                     "new_ratio": new_ratio, "len_ratio": len_ratio})

    # ---- 分组分布 ----
    groups = defaultdict(list)
    for r in rows:
        groups[r["expected"]].append(r["score"])

    print("=== 各期望动作的 top-1 相似度分布 ===")
    print(f"  {'action':11s} {'n':>2s} {'min':>7s} {'p25':>7s} {'median':>7s} {'p75':>7s} {'max':>7s}")
    for act in ["CREATE", "NOOP", "SUPERSEDE", "UPDATE", "REINFORCE"]:
        v = groups.get(act, [])
        if not v:
            continue
        a = np.array(v)
        print(f"  {act:11s} {len(v):2d} {a.min():7.3f} {np.percentile(a,25):7.3f} "
              f"{np.median(a):7.3f} {np.percentile(a,75):7.3f} {a.max():7.3f}")

    # ---- 逐场景明细（按分数升序，便于看分界）----
    print("\n=== 逐场景明细（按 top-1 分数升序）===")
    for r in sorted(rows, key=lambda x: x["score"]):
        tgt = "" if r["want_idx"] < 0 else f" want={r['want_idx']} got={r['top_idx']}"
        mark = "  <<<" if (r["want_idx"] >= 0 and r["top_idx"] != r["want_idx"]) else ""
        print(f"  {r['score']:6.3f}  {r['expected']:10s} {r['id']:14s}{tgt}{mark}  {r['cand'][:30]}")

    # ---- 目标定位正确率（多条 seed 时选对目标）----
    tot = sum(1 for r in rows if r["want_idx"] >= 0)
    ok = sum(1 for r in rows if r["want_idx"] >= 0 and r["top_idx"] == r["want_idx"])
    print(f"\n=== top-1 目标定位正确率: {ok}/{tot} ===")

    # ---- 类间可分性：找最佳二分阈值 ----
    print("\n=== 可分性分析（哪些类靠阈值分不开、必须交给 System-2）===")
    def rng(act):
        v = groups.get(act, [])
        return (min(v), max(v)) if v else (0.0, 0.0)
    for a, b in [("CREATE", "REINFORCE"), ("NOOP", "REINFORCE"),
                 ("UPDATE", "REINFORCE"), ("SUPERSEDE", "REINFORCE"),
                 ("CREATE", "SUPERSEDE"), ("UPDATE", "SUPERSEDE"), ("NOOP", "CREATE")]:
        lo_a, hi_a = rng(a)
        lo_b, hi_b = rng(b)
        overlap = not (hi_a < lo_b or hi_b < lo_a)
        print(f"  {a:10s}[{lo_a:.3f},{hi_a:.3f}] vs {b:10s}[{lo_b:.3f},{hi_b:.3f}]"
              f"  {'❌ 区间重叠→需 System-2' if overlap else '✅ 可阈值分开'}")

    # ---- 新颖度信号：能否把 UPDATE 从 REINFORCE/SUPERSEDE 里分出来 ----
    # Mem0 Algorithm 1：UPDATE 仅当 InformationContent(f) > InformationContent(m_i)。
    # 代理信号 = 候选相对匹配记忆的新 token 占比（new_ratio）。
    print("\n=== 新颖度信号（new_ratio = 候选新 token 占比）===")
    print(f"  {'action':11s} {'n':>2s} {'min':>7s} {'median':>7s} {'max':>7s}   {'len_ratio med':>14s}")
    for act in ["UPDATE", "REINFORCE", "SUPERSEDE"]:
        v = [r["new_ratio"] for r in rows if r["expected"] == act]
        lr = [r["len_ratio"] for r in rows if r["expected"] == act]
        if not v:
            continue
        a, b = np.array(v), np.array(lr)
        print(f"  {act:11s} {len(v):2d} {a.min():7.3f} {np.median(a):7.3f} {a.max():7.3f}"
              f"   {np.median(b):14.3f}")
    print("\n  逐场景 new_ratio（升序）:")
    for r in sorted(rows, key=lambda x: x["new_ratio"]):
        if r["expected"] in ("UPDATE", "REINFORCE", "SUPERSEDE"):
            print(f"    {r['new_ratio']:6.3f}  {r['expected']:10s} {r['id']:14s} {r['cand'][:28]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
