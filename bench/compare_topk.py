#!/usr/bin/env python3
"""P4：对比两次检索评测的 top-k 命中集合是否一致（INT8 vs FP32）。

F1 只测了 embedding 余弦（0.99），但**余弦高不代表排序不翻转**——这是当时留下的
方法论缺口。这里直接比 top-k 的 id 集合与顺序。

用法: python3 compare_topk.py <a.json> <b.json> [标签A] [标签B]
"""
import json
import sys


def main():
    if len(sys.argv) < 3:
        print("usage: compare_topk.py <a.json> <b.json> [labelA] [labelB]", file=sys.stderr)
        return 1
    pa, pb = sys.argv[1], sys.argv[2]
    la = sys.argv[3] if len(sys.argv) > 3 else "A"
    lb = sys.argv[4] if len(sys.argv) > 4 else "B"

    a = json.load(open(pa, encoding="utf-8"))
    b = json.load(open(pb, encoding="utf-8"))

    if a["domain"] != b["domain"]:
        print("域不一致，无法对比: %s vs %s" % (a["domain"], b["domain"]), file=sys.stderr)
        return 1

    qa, qb = a["queries"], b["queries"]
    if len(qa) != len(qb):
        print("查询数不一致: %d vs %d" % (len(qa), len(qb)), file=sys.stderr)
        return 1

    n = len(qa)
    exact_order = 0      # top-k 顺序完全一致
    same_set = 0         # top-k 集合一致（顺序可不同）
    set_diffs = []

    for i in range(n):
        ra, rb = qa[i]["retrieved"], qb[i]["retrieved"]
        if ra == rb:
            exact_order += 1
            same_set += 1
        elif set(ra) == set(rb):
            same_set += 1
        else:
            only_a = sorted(set(ra) - set(rb))
            only_b = sorted(set(rb) - set(ra))
            set_diffs.append((i, qa[i]["query"], ra, rb, only_a, only_b))

    print("=== %s vs %s  top-k 一致性（域=%s, n=%d, k=%d）===" % (la, lb, a["domain"], n, a["topk"]))
    print("  顺序完全一致: %d/%d (%.1f%%)" % (exact_order, n, 100.0 * exact_order / n))
    print("  集合一致    : %d/%d (%.1f%%)" % (same_set, n, 100.0 * same_set / n))
    print("  指标对比    : recall@k %.4f vs %.4f | MRR %.4f vs %.4f | hit@1 %.4f vs %.4f"
          % (a["recall_at_k"], b["recall_at_k"], a["mrr"], b["mrr"], a["hit_at_1"], b["hit_at_1"]))

    if set_diffs:
        print("  --- 集合不一致的查询（前 5 条）---")
        for i, q, ra, rb, oa, ob in set_diffs[:5]:
            print("    [%d] %s" % (i, q))
            print("        %s: %s" % (la, ra))
            print("        %s: %s" % (lb, rb))
            print("        仅%s有: %s   仅%s有: %s" % (la, oa, lb, ob))
    else:
        print("  ✅ 所有查询的 top-k 集合完全一致")

    return 0


if __name__ == "__main__":
    sys.exit(main())
