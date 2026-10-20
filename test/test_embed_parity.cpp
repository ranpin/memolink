#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include "models/WordPieceTokenizer.h"
#include "models/OnnxEmbedder.h"

// P2 embedding 对拍：C++ (WordPieceTokenizer + OnnxEmbedder) vs Python (tokenizers + onnxruntime)
// 两边用**同一个 INT8 模型**，所以期望余弦 ≈1.0000；明显低于 1 说明 C++ 侧推理有 bug。
//
// 用法: test_embed_parity <tokenizer.json> <model.onnx> <strings.txt> <out.txt> [threads]
int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "usage: " << argv[0]
                  << " <tokenizer.json> <model.onnx> <strings.txt> <out.txt> [threads]" << std::endl;
        return 1;
    }
    const std::string tok_path = argv[1], model_path = argv[2];
    const std::string strings_path = argv[3], out_path = argv[4];
    const int threads = argc > 5 ? std::atoi(argv[5]) : 4;

    WordPieceTokenizer tok;
    if (!tok.load(tok_path)) { std::cerr << "tokenizer load failed\n"; return 1; }

    OnnxEmbedder emb;
    if (!emb.load(model_path, threads)) { std::cerr << "onnx load failed\n"; return 1; }

    std::ifstream f(strings_path);
    if (!f.is_open()) { std::cerr << "cannot open " << strings_path << "\n"; return 1; }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    // 逐条编码（不截断），再右侧 padding 到 batch 内最长——与 Python enable_padding 一致
    std::vector<std::vector<int64_t>> raw;
    size_t maxL = 0;
    for (const auto& s : lines) {
        std::vector<int> ids = tok.encode(s, 0);
        raw.emplace_back(ids.begin(), ids.end());
        maxL = std::max(maxL, ids.size());
    }

    const int64_t pad = tok.padId();
    std::vector<std::vector<int64_t>> ids_b, mask_b, tt_b;
    ids_b.reserve(raw.size()); mask_b.reserve(raw.size()); tt_b.reserve(raw.size());
    for (auto v : raw) {
        const size_t L = v.size();
        std::vector<int64_t> m(L, 1), t(L, 0);
        v.resize(maxL, pad);
        m.resize(maxL, 0);
        t.resize(maxL, 0);
        ids_b.push_back(std::move(v));
        mask_b.push_back(std::move(m));
        tt_b.push_back(std::move(t));
    }

    std::vector<std::vector<float>> vecs = emb.run(ids_b, mask_b, tt_b);
    if (vecs.size() != lines.size()) {
        std::cerr << "embed count mismatch: got " << vecs.size() << " want " << lines.size() << "\n";
        return 1;
    }

    std::ofstream out(out_path);
    out << std::setprecision(8);
    for (const auto& v : vecs) {
        for (size_t i = 0; i < v.size(); ++i) out << (i ? " " : "") << v[i];
        out << "\n";
    }
    std::cerr << "[cpp] embedded " << vecs.size() << " strings, dim=" << emb.dim()
              << ", maxL=" << maxL << std::endl;
    return 0;
}
