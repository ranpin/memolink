#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// bge-small-zh-v1.5 的 WordPiece 分词器（C++ 复刻，照 tokenizer.json 的配置实现）。
//
// 关键配置（从 tokenizer.json 读出，勿凭印象改）：
//   BertNormalizer { clean_text:true, handle_chinese_chars:true,
//                    strip_accents:null, lowercase:false }
//   BertPreTokenizer（按 \w+|[^\w\s]+ 切 pre-token）
//   WordPiece { unk:"[UNK]", prefix:"##", max_input_chars_per_word:100 }
//   post-process: [CLS] + seq + [SEP]
//
// handle_chinese_chars 会给每个 CJK 字符两侧补空格，使其成为独立 pre-token；
// lowercase=false，不做小写化。这两点错了中文会全切错。
class WordPieceTokenizer {
public:
    // 从 tokenizer.json 加载词表与配置。失败返回 false。
    bool load(const std::string& tokenizer_json_path);

    // 编码单条序列，返回含 [CLS]...[SEP] 的 token id。
    // max_length>0 时截断到 max_length（含两个特殊符）。
    std::vector<int> encode(const std::string& text, int max_length = 0) const;

    // 只做 normalize + pre-tokenize + WordPiece，不加特殊符（供对拍调试）。
    std::vector<int> tokenizeNoSpecial(const std::string& text) const;

    // 返回 pre-token 字符串（供对拍调试）。
    std::vector<std::string> preTokens(const std::string& text) const;

    int vocabSize() const { return vocab_size_; }
    int clsId() const { return cls_id_; }
    int sepId() const { return sep_id_; }
    int padId() const { return pad_id_; }
    int unkId() const { return unk_id_; }

private:
    std::unordered_map<std::string, int> vocab_;
    int vocab_size_ = 0;
    std::string unk_token_ = "[UNK]";
    std::string continuing_prefix_ = "##";
    int max_input_chars_per_word_ = 100;
    int pad_id_ = 0, unk_id_ = 100, cls_id_ = 101, sep_id_ = 102;
    bool lowercase_ = false;
    bool handle_chinese_chars_ = true;
    bool clean_text_ = true;

    std::string normalize(const std::string& text) const;
    std::vector<std::string> preTokenize(const std::string& normalized) const;
    std::vector<int> wordpieceToken(const std::string& token) const;
};
