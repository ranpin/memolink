#!/usr/bin/env python3
"""F2：检索层规模实验 —— numpy 暴力 vs faiss 精确 vs faiss 近似（HNSW / IVF）。

要回答的问题不是「faiss 快不快」，而是**端侧 agent 记忆到底需不需要 ANN**：
agent 记忆的现实量级是 1e3~1e4 条，如果这个规模上精确暴力检索已经够快，
那么上 ANN 就是过度设计——既引入 recall 损失，又引入索引维护成本。

在 N = 1k / 10k / 100k 上测：
- numpy ：矩阵乘暴力（精确），端侧最省依赖的基线
- flat  ：faiss IndexFlatIP（精确，SIMD 优化）
- hnsw  ：faiss IndexHNSWFlat（近似，图索引，扫 efSearch）
- ivf   ：faiss IndexIVFFlat（近似，倒排，扫 nprobe）

指标：建索引耗时、索引常驻内存、单查询 top-k 延迟、recall@k
（以 numpy 精确结果为 ground truth）。

测量纪律（四处踩过坑，这里显式规避）：
- 索引内存用**序列化文件大小**，不用 RSS 增量——同进程里连续建/删索引时，
  前一个索引释放的页会被后一个复用，resident 集合几乎不涨，RSS 增量测不准
  （HNSW 曾因此测出 0.0 的假值，而它真实占用 ~233MB@100k）。
- efSearch / nprobe 是**查询期**参数，建一次索引扫多个取值即可；每个取值都
  重建索引既慢数倍，也会把 build_s 重复计入。
- IVF 的 nlist/nprobe 必须随 N 适配并**如实报出**：nlist=sqrt(N) 在 N=1k 时
  只有 31 个簇、训练点不足（faiss 会警告），nprobe 太小会让 recall 崩到 0.1，
  那是参数错误而不是 IVF 的固有特性。

注：向量是**带簇结构的合成数据**（queries 与 base 共享同一组簇中心 + 小噪声）——
比纯随机更接近真实 embedding 的分布，让 ANN 的 recall 有代表性。纯随机高维向量
没有簇结构、是 ANN 的最坏情形（会把 recall 压到 0.4 以下），不能用来评判 ANN；
而精确检索的延迟与数据分布无关，核心结论不受此影响。「检索找得对不对」的语义质量
在 F3 用真实记忆语料测。
"""
import argparse
import json
import os
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
RESULT_DIR = os.path.normpath(os.path.join(HERE, "..", "results"))

DIM = 512          # bge-small-zh-v1.5 的 embedding 维度（F1 实测）
K = 5
N_QUERIES = 100
SCALES = [1_000, 10_000, 100_000]
SEED = 20261020
FAISS_THREADS = 4
HNSW_EF_SEARCH = [16, 64, 256]
IVF_NPROBE = [1, 4, 16, 64]
N_CLUSTERS = 256   # queries 与所有 base 共享的簇中心数
CLUSTER_NOISE = 0.02  # 簇内噪声 std；相对单位簇中心要小，保证簇可分但非平凡


def index_size_mb(index, faiss):
    """索引大小 = 序列化到临时文件的字节数——确定性度量。

    比 RSS 增量可靠：同进程连续建/删索引时页会被复用，RSS 增量测不准。
    """
    import tempfile

    fd, path = tempfile.mkstemp(suffix=".faiss", dir="/tmp")
    os.close(fd)
    try:
        faiss.write_index(index, path)
        return os.path.getsize(path) / 1e6
    finally:
        os.remove(path)


def make_centers(rng):
    """共享簇中心：queries 与所有 base 都从这组中心采样，检索才有意义。

    若 queries 与 base 各自独立随机生成簇中心，查询点会离所有库点都很远，
    top-k 退化成噪声，recall 数字也就没有意义了。
    """
    c = rng.standard_normal((N_CLUSTERS, DIM), dtype=np.float32)
    c /= np.linalg.norm(c, axis=1, keepdims=True) + 1e-9
    return c


def make_clustered(n, rng, centers):
    """从共享簇中心采样 + 小簇内噪声，再 L2 归一化。

    高维下随机单位向量彼此近似正交（中心间距 ~sqrt(2)），噪声 std=0.02 时
    簇内距离远小于簇间距——有真实 embedding 那样的簇结构，但不是平凡可分。
    """
    labels = rng.integers(0, len(centers), size=n)
    v = centers[labels] + CLUSTER_NOISE * rng.standard_normal((n, DIM), dtype=np.float32)
    v /= np.linalg.norm(v, axis=1, keepdims=True) + 1e-9
    return v.astype(np.float32)


def gt_search(base, queries, k):
    sims = queries @ base.T
    return np.argsort(-sims, axis=1)[:, :k]


def recall_at_k(got, truth, k):
    hits = sum(len(set(g[:k].tolist()) & set(t[:k].tolist())) for g, t in zip(got, truth))
    return hits / (len(truth) * k)


def timed_search(index, queries, k):
    t0 = time.perf_counter()
    _, idx = index.search(queries, k)
    return idx, (time.perf_counter() - t0) * 1000.0 / len(queries)


def rec(idx, qms, build_s, mem_mb, truth, params=None, exact=False):
    d = {
        "query_ms": round(qms, 4),
        "build_s": round(build_s, 3),
        "index_mb": round(mem_mb, 2),
        "recall": round(recall_at_k(idx, truth, K), 5),
        "exact": exact,
    }
    if params:
        d["params"] = params
    return d


def bench_exact(base, queries, truth, faiss):
    """numpy 暴力 与 faiss IndexFlatIP，两者都是精确检索（recall 恒为 1）。"""
    out = {}

    t0 = time.perf_counter()
    sims = queries @ base.T
    idx_np = np.argsort(-sims, axis=1)[:, :K]
    qms = (time.perf_counter() - t0) * 1000.0 / len(queries)
    out["numpy"] = rec(idx_np, qms, 0.0, 0.0, truth, exact=True)

    t0 = time.perf_counter()
    index = faiss.IndexFlatIP(DIM)
    index.add(base)
    build_s = time.perf_counter() - t0
    mem_mb = index_size_mb(index, faiss)
    idx, qms = timed_search(index, queries, K)
    out["flat"] = rec(idx, qms, build_s, mem_mb, truth, exact=True)
    del index
    return out


def bench_hnsw(base, queries, truth, faiss):
    """建一次图索引，扫查询期 efSearch。"""
    t0 = time.perf_counter()
    index = faiss.IndexHNSWFlat(DIM, 32, faiss.METRIC_INNER_PRODUCT)
    index.hnsw.efConstruction = 200
    index.add(base)
    build_s = time.perf_counter() - t0
    mem_mb = index_size_mb(index, faiss)

    out = {"M": 32, "efConstruction": 200, "build_s": round(build_s, 3),
           "index_mb": round(mem_mb, 2), "sweeps": {}}
    for ef in HNSW_EF_SEARCH:
        index.hnsw.efSearch = ef
        idx, qms = timed_search(index, queries, K)
        out["sweeps"][f"efSearch={ef}"] = rec(
            idx, qms, build_s, mem_mb, truth, params={"efSearch": ef}
        )
    del index
    return out


def bench_ivf(base, queries, truth, faiss, nlist):
    """建一次倒排索引，扫查询期 nprobe。"""
    t0 = time.perf_counter()
    quantizer = faiss.IndexFlatIP(DIM)
    index = faiss.IndexIVFFlat(quantizer, DIM, nlist, faiss.METRIC_INNER_PRODUCT)
    index.train(base)
    index.add(base)
    build_s = time.perf_counter() - t0
    mem_mb = index_size_mb(index, faiss)

    out = {"nlist": nlist, "build_s": round(build_s, 3),
           "index_mb": round(mem_mb, 2), "sweeps": {}}
    for nprobe in IVF_NPROBE:
        if nprobe > nlist:
            continue
        index.nprobe = nprobe
        idx, qms = timed_search(index, queries, K)
        out["sweeps"][f"nprobe={nprobe}"] = rec(
            idx, qms, build_s, mem_mb, truth, params={"nprobe": nprobe}
        )
    del index
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scales", type=int, nargs="*", default=SCALES)
    args = ap.parse_args()

    import faiss
    faiss.omp_set_num_threads(FAISS_THREADS)

    rng = np.random.default_rng(SEED)
    centers = make_centers(rng)
    queries = make_clustered(N_QUERIES, rng, centers)
    out = {
        "dim": DIM, "k": K, "n_queries": N_QUERIES, "seed": SEED,
        "faiss_threads": FAISS_THREADS, "faiss_version": faiss.__version__,
        "data": f"clustered synthetic: {N_CLUSTERS} shared centers, noise std={CLUSTER_NOISE}",
        "note": "语义检索质量在 F3 用真实记忆语料测",
        "scales": [],
    }

    for n in args.scales:
        base = make_clustered(n, rng, centers)
        truth = gt_search(base, queries, K)
        nlist = max(4, min(int(np.sqrt(n)), 1024))

        entry = {"n": n, "base_mb": round(base.nbytes / 1e6, 1), "methods": {}}
        entry["methods"].update(bench_exact(base, queries, truth, faiss))
        entry["methods"]["hnsw"] = bench_hnsw(base, queries, truth, faiss)
        entry["methods"]["ivf"] = bench_ivf(base, queries, truth, faiss, nlist)
        out["scales"].append(entry)

        m = entry["methods"]
        print(f"\n=== N={n:,}  base={entry['base_mb']}MB  (ivf nlist={nlist}) ===", flush=True)
        print(f"  {'method':20s} {'query_ms':>9s} {'build_s':>8s} {'index_mb':>9s} {'recall@5':>9s}")
        for name in ("numpy", "flat"):
            r = m[name]
            print(f"  {name:20s} {r['query_ms']:9.4f} {r['build_s']:8.3f} {r['index_mb']:9.2f} {r['recall']:9.5f}")
        for label, r in m["hnsw"]["sweeps"].items():
            print(f"  {'hnsw '+label:20s} {r['query_ms']:9.4f} {r['build_s']:8.3f} {r['index_mb']:9.2f} {r['recall']:9.5f}")
        for label, r in m["ivf"]["sweeps"].items():
            print(f"  {'ivf '+label:20s} {r['query_ms']:9.4f} {r['build_s']:8.3f} {r['index_mb']:9.2f} {r['recall']:9.5f}")
        del base, truth

    os.makedirs(RESULT_DIR, exist_ok=True)
    path = os.path.join(RESULT_DIR, "f2_retrieval.json")
    with open(path, "w") as f:
        json.dump(out, f, ensure_ascii=False, indent=2)
    print(f"\n写入 {path}", flush=True)
    print("F2 DONE", flush=True)


if __name__ == "__main__":
    main()
