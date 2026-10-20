#!/usr/bin/env python3
"""P2 对拍参照：用 HF tokenizers 编码同一份字符串文件，输出格式与 C++ 版一致。

用法: python3 tokenizer_parity.py <tokenizer.json> <strings.txt> [max_len]
输出: 每行 "<idx>: <id> <id> ..."
"""
import sys

from tokenizers import Tokenizer


def main():
    if len(sys.argv) < 3:
        print("usage: tokenizer_parity.py <tokenizer.json> <strings.txt> [max_len]", file=sys.stderr)
        return 1
    tok_path, strings_path = sys.argv[1], sys.argv[2]
    max_len = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    tok = Tokenizer.from_file(tok_path)
    if max_len > 0:
        tok.enable_truncation(max_length=max_len)

    print(f"[py] vocab={tok.get_vocab_size()}", file=sys.stderr)

    with open(strings_path, encoding="utf-8") as f:
        lines = [l.rstrip("\n").rstrip("\r") for l in f]

    for idx, line in enumerate(lines):
        enc = tok.encode(line)
        print(f"{idx}: " + " ".join(str(i) for i in enc.ids))
    return 0


if __name__ == "__main__":
    sys.exit(main())
