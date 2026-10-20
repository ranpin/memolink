#!/usr/bin/env python3
"""P2 embedding 对拍参照：Python (tokenizers + onnxruntime) 侧。

与 test/test_embed_parity.cpp 用同一个 INT8 模型、同一份字符串、同样的右侧 padding，
输出同格式（每行一个 L2 归一化向量，空格分隔）。期望两边余弦 ≈1.0000。

用法: python3 embed_parity.py <tokenizer.json> <model.onnx> <strings.txt> <out.txt> [threads]
"""
import sys

import numpy as np
import onnxruntime as ort
from tokenizers import Tokenizer


def main():
    if len(sys.argv) < 5:
        print("usage: embed_parity.py <tokenizer.json> <model.onnx> <strings.txt> <out.txt> [threads]",
              file=sys.stderr)
        return 1
    tok_path, model_path, strings_path, out_path = sys.argv[1:5]
    threads = int(sys.argv[5]) if len(sys.argv) > 5 else 4

    tok = Tokenizer.from_file(tok_path)
    pad_id = tok.token_to_id("[PAD]")
    # 右侧 padding 到 batch 内最长，与 C++ 侧一致；不截断
    tok.enable_padding(direction="right", pad_id=pad_id, pad_token="[PAD]")

    so = ort.SessionOptions()
    so.intra_op_num_threads = threads
    so.inter_op_num_threads = 1
    so.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    sess = ort.InferenceSession(model_path, sess_options=so, providers=["CPUExecutionProvider"])
    in_names = [i.name for i in sess.get_inputs()]
    out_names = [o.name for o in sess.get_outputs()]

    with open(strings_path, encoding="utf-8") as f:
        lines = [l.rstrip("\n").rstrip("\r") for l in f]

    enc = tok.encode_batch(lines)
    ids = np.array([e.ids for e in enc], dtype=np.int64)
    mask = np.array([e.attention_mask for e in enc], dtype=np.int64)
    tt = np.array([e.type_ids for e in enc], dtype=np.int64)

    feed = {}
    if "input_ids" in in_names:
        feed["input_ids"] = ids
    if "attention_mask" in in_names:
        feed["attention_mask"] = mask
    if "token_type_ids" in in_names:
        feed["token_type_ids"] = tt

    out = sess.run(None, feed)
    oi = out_names.index("sentence_embedding") if "sentence_embedding" in out_names else 0
    emb = np.asarray(out[oi], dtype=np.float32)
    emb = emb / (np.linalg.norm(emb, axis=1, keepdims=True) + 1e-9)

    with open(out_path, "w", encoding="utf-8") as f:
        for v in emb:
            f.write(" ".join(f"{x:.8g}" for x in v) + "\n")
    print(f"[py] embedded {len(emb)} dim={emb.shape[1]} seq_len={ids.shape[1]}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
