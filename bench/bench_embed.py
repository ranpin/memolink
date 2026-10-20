#!/usr/bin/env python3
"""F1 基准：bge-small-zh-v1.5 ONNX 在 Orin CPU 上的 FP32 vs INT8 对比。

设计要点（都是为了数字可信）：
- 每个 (变体, 线程数) 组合在**独立子进程**里测——ru_maxrss 是进程峰值，
  同进程先后跑两个模型会让后一个的 RSS 被前一个污染。
- 模型磁盘大小**必须含 ONNX external data**（.onnx 只是图，权重在同名 _data 里）。
- RSS 同时报 baseline（import 完、建 session 前）与 peak，差值才是模型占用。
- embedding 优先用模型自带的 sentence_embedding 输出，而不是手动取 CLS。
- 测 batch=1（agent 单条查询的真实场景）与 batch=N（入库批量编码）两种延迟。
- 扫线程数：端侧要和别的负载共存，限制 intra-op 线程是资源调度的关键旋钮。
"""
import argparse
import json
import os
import resource
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
MODEL_DIR = os.path.normpath(os.path.join(HERE, "..", "models"))
RESULT_DIR = os.path.normpath(os.path.join(HERE, "..", "results"))

VARIANTS = {"fp32": "model.onnx", "int8": "model_quantized.onnx"}

# BGE 检索约定：query 侧带指令前缀，memory 侧不带。
QUERY_PREFIX = "为这个句子生成表示以用于检索相关文章："
QUERIES = [
    "用户喜欢什么颜色的车？",
    "上次保养是什么时候？",
    "帮我把空调调到 22 度",
    "明天杭州的天气怎么样",
]
MEMORIES = [
    "用户说过他喜欢蓝色的车，尤其是深蓝。",
    "车辆上次保养是 2026 年 8 月，里程 12000 公里。",
    "空调当前设定 24 度，风量 2 档。",
    "用户住在杭州西湖区。",
]


def rss_mb():
    """Linux 上 ru_maxrss 单位是 KB。"""
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0


def model_size_mb(onnx_path):
    """.onnx 只是计算图，权重在同名 _data 外部文件里，必须一起算。"""
    total = os.path.getsize(onnx_path)
    parts = [os.path.basename(onnx_path)]
    data = onnx_path + "_data"
    if os.path.exists(data):
        total += os.path.getsize(data)
        parts.append(os.path.basename(data))
    return round(total / 1e6, 2), parts


def build_inputs(tok_path, texts, with_prefix, max_len=128):
    from tokenizers import Tokenizer

    tok = Tokenizer.from_file(tok_path)
    pad_id = tok.token_to_id("[PAD]")
    tok.enable_padding(direction="right", pad_id=pad_id, pad_token="[PAD]")
    tok.enable_truncation(max_length=max_len)
    if with_prefix:
        texts = [QUERY_PREFIX + t for t in texts]
    enc = tok.encode_batch(texts)
    return (
        np.array([e.ids for e in enc], dtype=np.int64),
        np.array([e.attention_mask for e in enc], dtype=np.int64),
        np.array([e.type_ids for e in enc], dtype=np.int64),
    )


def pool(sess, out, out_names):
    """优先用模型自带的 sentence_embedding；否则退回 CLS + L2 归一化。"""
    if "sentence_embedding" in out_names:
        emb = out[out_names.index("sentence_embedding")]
        src = "sentence_embedding"
    else:
        emb = out[0][:, 0, :]
        src = "cls"
    emb = np.asarray(emb, dtype=np.float32)
    emb = emb / (np.linalg.norm(emb, axis=1, keepdims=True) + 1e-9)
    return emb, src


def time_runs(sess, feed, n):
    t0 = time.perf_counter()
    for _ in range(n):
        sess.run(None, feed)
    return (time.perf_counter() - t0) / n * 1000.0


def measure(variant, threads):
    import onnxruntime as ort

    baseline_rss = rss_mb()
    onnx_path = os.path.join(MODEL_DIR, VARIANTS[variant])
    tok_path = os.path.join(MODEL_DIR, "tokenizer.json")
    size_mb, size_parts = model_size_mb(onnx_path)

    so = ort.SessionOptions()
    so.intra_op_num_threads = threads
    so.inter_op_num_threads = 1
    so.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL

    t0 = time.perf_counter()
    sess = ort.InferenceSession(onnx_path, sess_options=so, providers=["CPUExecutionProvider"])
    load_s = time.perf_counter() - t0

    in_names = [i.name for i in sess.get_inputs()]
    out_names = [o.name for o in sess.get_outputs()]

    def feed_for(texts, with_prefix):
        ids, mask, tt = build_inputs(tok_path, texts, with_prefix)
        feed = {}
        if "input_ids" in in_names:
            feed["input_ids"] = ids
        if "attention_mask" in in_names:
            feed["attention_mask"] = mask
        if "token_type_ids" in in_names:
            feed["token_type_ids"] = tt
        return feed

    q_feed = feed_for(QUERIES, with_prefix=True)
    m_feed = feed_for(MEMORIES, with_prefix=False)
    single_feed = feed_for(QUERIES[:1], with_prefix=True)

    for _ in range(5):  # warmup
        sess.run(None, q_feed)

    out_q = sess.run(None, q_feed)
    out_m = sess.run(None, m_feed)
    emb_q, pool_src = pool(sess, out_q, out_names)
    emb_m, _ = pool(sess, out_m, out_names)

    n_single = 100
    n_batch = 50
    res = {
        "variant": variant,
        "threads": threads,
        "file": VARIANTS[variant],
        "size_mb": size_mb,
        "size_parts": size_parts,
        "load_s": round(load_s, 3),
        "single_query_ms": round(time_runs(sess, single_feed, n_single), 3),
        "batch4_query_ms": round(time_runs(sess, q_feed, n_batch), 3),
        "batch4_memory_ms": round(time_runs(sess, m_feed, n_batch), 3),
        "dim": int(emb_q.shape[1]),
        "pool_source": pool_src,
        "inputs": in_names,
        "outputs": out_names,
        "rss_baseline_mb": round(baseline_rss, 1),
        "rss_peak_mb": round(rss_mb(), 1),
    }
    res["rss_model_mb"] = round(res["rss_peak_mb"] - res["rss_baseline_mb"], 1)

    os.makedirs(RESULT_DIR, exist_ok=True)
    np.save(os.path.join(RESULT_DIR, f"emb_{variant}_q.npy"), emb_q)
    np.save(os.path.join(RESULT_DIR, f"emb_{variant}_m.npy"), emb_m)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--variant", choices=list(VARIANTS))
    ap.add_argument("--threads", type=int, default=4)
    ap.add_argument("--all", action="store_true", help="用子进程逐个测，输出汇总 JSON")
    args = ap.parse_args()

    if not args.all:
        print(json.dumps(measure(args.variant, args.threads), ensure_ascii=False))
        return

    thread_grid = [1, 2, 4, 8]
    runs = []
    for variant in VARIANTS:
        for th in thread_grid:
            cmd = [sys.executable, os.path.abspath(__file__), "--variant", variant, "--threads", str(th)]
            p = subprocess.run(cmd, capture_output=True, text=True)
            if p.returncode != 0:
                print(f"[FAIL] {variant} threads={th}\n{p.stderr[-800:]}", file=sys.stderr)
                continue
            line = [l for l in p.stdout.strip().splitlines() if l.startswith("{")]
            runs.append(json.loads(line[-1]))
            r = runs[-1]
            print(
                f"  {r['variant']:5s} th={r['threads']:2d}  "
                f"single={r['single_query_ms']:6.2f}ms  batch4={r['batch4_query_ms']:6.2f}ms  "
                f"size={r['size_mb']:6.1f}MB  rss_model={r['rss_model_mb']:6.1f}MB",
                flush=True,
            )

    # 量化质量：同输入下 INT8 与 FP32 embedding 的余弦（取 threads=4 那次）
    qual = {}
    for split in ("q", "m"):
        a = np.load(os.path.join(RESULT_DIR, f"emb_fp32_{split}.npy"))
        b = np.load(os.path.join(RESULT_DIR, f"emb_int8_{split}.npy"))
        cos = (a * b).sum(axis=1)
        qual[split] = {"mean": round(float(cos.mean()), 5), "min": round(float(cos.min()), 5)}

    summary = {
        "device": "Orin (dog) aarch64, onnxruntime CPUExecutionProvider",
        "model": "bge-small-zh-v1.5",
        "runs": runs,
        "int8_vs_fp32_cosine": qual,
    }
    os.makedirs(RESULT_DIR, exist_ok=True)
    with open(os.path.join(RESULT_DIR, "f1_embed.json"), "w") as f:
        json.dump(summary, f, ensure_ascii=False, indent=2)
    print("\n=== INT8 vs FP32 embedding 余弦 ===")
    print(json.dumps(qual, ensure_ascii=False, indent=2))
    print(f"\n写入 {os.path.join(RESULT_DIR, 'f1_embed.json')}")


if __name__ == "__main__":
    main()
