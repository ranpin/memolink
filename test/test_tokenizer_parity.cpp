#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "models/WordPieceTokenizer.h"

// P2 对拍工具：读入字符串文件，逐行输出 token id，格式与 bench/tokenizer_parity.py 一致。
// 用法: test_tokenizer_parity <tokenizer.json> <strings.txt> [max_len]
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " <tokenizer.json> <strings.txt> [max_len]" << std::endl;
        return 1;
    }
    const std::string tok_path = argv[1];
    const std::string strings_path = argv[2];
    const int max_len = argc > 3 ? std::atoi(argv[3]) : 0;

    WordPieceTokenizer tok;
    if (!tok.load(tok_path)) {
        std::cerr << "load tokenizer failed: " << tok_path << std::endl;
        return 1;
    }
    std::cerr << "[cpp] vocab=" << tok.vocabSize()
              << " cls=" << tok.clsId() << " sep=" << tok.sepId()
              << " unk=" << tok.unkId() << " pad=" << tok.padId() << std::endl;

    std::ifstream f(strings_path);
    if (!f.is_open()) {
        std::cerr << "cannot open " << strings_path << std::endl;
        return 1;
    }
    std::string line;
    int idx = 0;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // CRLF 容错
        std::vector<int> ids = tok.encode(line, max_len);
        std::cout << idx << ":";
        for (size_t i = 0; i < ids.size(); ++i)
            std::cout << (i ? " " : " ") << ids[i];
        std::cout << "\n";
        ++idx;
    }
    return 0;
}
