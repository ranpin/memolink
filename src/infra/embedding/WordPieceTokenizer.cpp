#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include "infra/embedding/WordPieceTokenizer.h"
#include <json/json.h>

// ---------------------------------------------------------------------------
// UTF-8 编解码（按码点处理，WordPiece 的迭代单位是字符不是字节）
// ---------------------------------------------------------------------------
static std::vector<uint32_t> utf8Decode(const std::string& s) {
    std::vector<uint32_t> out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp = 0;
        size_t len = 1;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 4; }
        else { cp = 0xFFFD; len = 1; }
        for (size_t j = 1; j < len && i + j < s.size(); ++j)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
        out.push_back(cp);
        i += len;
    }
    return out;
}

static std::string utf8Encode(uint32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

static std::string utf8EncodeRange(const std::vector<uint32_t>& cps, size_t start, size_t end) {
    std::string out;
    for (size_t i = start; i < end && i < cps.size(); ++i)
        out += utf8Encode(cps[i]);
    return out;
}

// ---------------------------------------------------------------------------
// 字符分类（近似 Unicode 类别，覆盖 ASCII + CJK + 常见脚本；对拍测试驱动修正）
// ---------------------------------------------------------------------------
static bool is_control(uint32_t cp) {
    if (cp == '\t' || cp == '\n' || cp == '\r') return false;
    if ((cp <= 0x1F) || cp == 0x7F || (cp >= 0x80 && cp <= 0x9F)) return true;
    if (cp == 0xFFFD) return true;
    // 常见 Cf（format）字符
    if (cp == 0xAD || cp == 0x200B || (cp >= 0x200C && cp <= 0x200F) ||
        (cp >= 0x202A && cp <= 0x202E) || cp == 0xFEFF ||
        (cp >= 0x2060 && cp <= 0x2064) || (cp >= 0xFFF9 && cp <= 0xFFFB) ||
        (cp >= 0x600 && cp <= 0x605) || cp == 0x61C || cp == 0x6DD || cp == 0x70F)
        return true;
    return false;
}

// 与 HF 的 \s 集合一致
static bool is_whitespace(uint32_t cp) {
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') return true;
    if (cp == 0x85 || cp == 0xA0 || cp == 0x1680) return true;
    if (cp >= 0x2000 && cp <= 0x200A) return true;
    if (cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F || cp == 0x3000) return true;
    return false;
}

// 与 HF BertNormalizer / 原始 BERT 的 CJK 区间一致
static bool is_chinese_char(uint32_t cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0x20000 && cp <= 0x2A6DF) ||
           (cp >= 0x2A700 && cp <= 0x2B73F) ||
           (cp >= 0x2B740 && cp <= 0x2B81F) ||
           (cp >= 0x2B820 && cp <= 0x2CEAF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0x2F800 && cp <= 0x2FA1F);
}

// \w（Unicode word char）的近似：字母 / 数字 / 下划线 / 组合标记
static bool is_word_char(uint32_t cp) {
    if ((cp >= '0' && cp <= '9') || (cp >= 'a' && cp <= 'z') ||
        (cp >= 'A' && cp <= 'Z') || cp == '_')
        return true;
    if (is_chinese_char(cp)) return true;
    if (cp >= 0x00C0 && cp <= 0x024F) return true;  // Latin-1 / Latin Extended
    if (cp >= 0x0300 && cp <= 0x036F) return true;  // 组合变音标记 (Mn)
    if (cp >= 0x0370 && cp <= 0x03FF) return true;  // Greek
    if (cp >= 0x0400 && cp <= 0x052F) return true;  // Cyrillic(+Supplement)
    if (cp >= 0x0530 && cp <= 0x058F) return true;  // Armenian
    if (cp >= 0x0590 && cp <= 0x05FF) return true;  // Hebrew
    if (cp >= 0x0600 && cp <= 0x06FF) return true;  // Arabic
    if (cp >= 0x0900 && cp <= 0x097F) return true;  // Devanagari
    if (cp >= 0x1E00 && cp <= 0x1EFF) return true;  // Latin Extended Additional
    if (cp >= 0x3040 && cp <= 0x309F) return true;  // Hiragana
    if (cp >= 0x30A0 && cp <= 0x30FF) return true;  // Katakana
    if (cp >= 0xAC00 && cp <= 0xD7AF) return true;  // Hangul
    if (cp >= 0xFF10 && cp <= 0xFF19) return true;  // 全角数字
    if (cp >= 0xFF21 && cp <= 0xFF3A) return true;  // 全角大写
    if (cp >= 0xFF41 && cp <= 0xFF5A) return true;  // 全角小写
    return false;
}

// ---------------------------------------------------------------------------
// 加载
// ---------------------------------------------------------------------------
bool WordPieceTokenizer::load(const std::string& tokenizer_json_path) {
    std::ifstream f(tokenizer_json_path, std::ios::binary);
    if (!f.is_open()) return false;
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, f, &root, &errs)) return false;

    const Json::Value& model = root["model"];
    if (model.isMember("unk_token")) unk_token_ = model["unk_token"].asString();
    if (model.isMember("continuing_subword_prefix"))
        continuing_prefix_ = model["continuing_subword_prefix"].asString();
    if (model.isMember("max_input_chars_per_word"))
        max_input_chars_per_word_ = model["max_input_chars_per_word"].asInt();

    const Json::Value& vocab = model["vocab"];
    for (auto it = vocab.begin(); it != vocab.end(); ++it)
        vocab_[it.key().asString()] = it->asInt();
    vocab_size_ = static_cast<int>(vocab_.size());
    if (vocab_size_ == 0) return false;

    const Json::Value& norm = root["normalizer"];
    if (norm.isMember("lowercase")) lowercase_ = norm["lowercase"].asBool();
    if (norm.isMember("handle_chinese_chars")) handle_chinese_chars_ = norm["handle_chinese_chars"].asBool();
    if (norm.isMember("clean_text")) clean_text_ = norm["clean_text"].asBool();

    if (vocab_.count("[PAD]")) pad_id_ = vocab_["[PAD]"];
    if (vocab_.count("[UNK]")) unk_id_ = vocab_["[UNK]"];
    if (vocab_.count("[CLS]")) cls_id_ = vocab_["[CLS]"];
    if (vocab_.count("[SEP]")) sep_id_ = vocab_["[SEP]"];
    return true;
}

// ---------------------------------------------------------------------------
// normalize：clean_text + handle_chinese_chars（lowercase=false，不转小写）
// ---------------------------------------------------------------------------
std::string WordPieceTokenizer::normalize(const std::string& text) const {
    auto cps = utf8Decode(text);

    if (clean_text_) {
        std::vector<uint32_t> cleaned;
        cleaned.reserve(cps.size());
        for (uint32_t cp : cps) {
            if (cp == 0 || cp == 0xFFFD || is_control(cp)) continue;
            cleaned.push_back(is_whitespace(cp) ? static_cast<uint32_t>(' ') : cp);
        }
        cps.swap(cleaned);
    }

    if (handle_chinese_chars_) {
        std::vector<uint32_t> handled;
        handled.reserve(cps.size() + 16);
        for (uint32_t cp : cps) {
            if (is_chinese_char(cp)) {
                handled.push_back(' ');
                handled.push_back(cp);
                handled.push_back(' ');
            } else {
                handled.push_back(cp);
            }
        }
        cps.swap(handled);
    }

    if (lowercase_) {
        for (auto& cp : cps)
            if (cp >= 'A' && cp <= 'Z') cp = cp - 'A' + 'a';
    }

    std::string result;
    result.reserve(cps.size());
    for (uint32_t cp : cps) result += utf8Encode(cp);
    return result;
}

// ---------------------------------------------------------------------------
// pre-tokenize：按 \w+ | [^\w\s] 切（空白丢弃），与 HF BertPreTokenizer 一致。
//
// ⚠️ 标点是**逐个**切、不是成团：HF / 原始 BERT 的 _run_split_on_punc 把每个标点
// 单独作为一个 pre-token，所以 "？？" → ["？","？"] 而不是 ["？？"]。
// 第一版写成 [^\w\s]+（成团），25 条对拍里错了 3 条，且全部是连续标点场景
// （"真的吗？？" / "A,B,,C...D" / "（括号）【方括号】"）——成团后 WordPiece 会把
// 第二个标点当续接子词去查 "##，"，得到与参照完全不同的 id。
// ---------------------------------------------------------------------------
std::vector<std::string> WordPieceTokenizer::preTokenize(const std::string& normalized) const {
    auto cps = utf8Decode(normalized);
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < cps.size()) {
        if (is_whitespace(cps[i])) { ++i; continue; }
        size_t start = i;
        if (is_word_char(cps[i])) {
            while (i < cps.size() && is_word_char(cps[i])) ++i;
        } else {
            ++i;  // 每个标点/符号单独成一个 pre-token
        }
        tokens.push_back(utf8EncodeRange(cps, start, i));
    }
    return tokens;
}

// ---------------------------------------------------------------------------
// WordPiece 贪心最长匹配
// ---------------------------------------------------------------------------
std::vector<int> WordPieceTokenizer::wordpieceToken(const std::string& token) const {
    auto cps = utf8Decode(token);
    if (static_cast<int>(cps.size()) > max_input_chars_per_word_)
        return {unk_id_};

    std::vector<int> result;
    size_t start = 0;
    while (start < cps.size()) {
        size_t end = cps.size();
        int found_id = -1;
        while (start < end) {
            std::string substr = utf8EncodeRange(cps, start, end);
            if (start > 0) substr = continuing_prefix_ + substr;
            auto it = vocab_.find(substr);
            if (it != vocab_.end()) { found_id = it->second; break; }
            --end;
        }
        if (found_id < 0) return {unk_id_};  // 整词回退 [UNK]
        result.push_back(found_id);
        start = end;
    }
    return result;
}

// ---------------------------------------------------------------------------
// 对外接口
// ---------------------------------------------------------------------------
std::vector<std::string> WordPieceTokenizer::preTokens(const std::string& text) const {
    return preTokenize(normalize(text));
}

std::vector<int> WordPieceTokenizer::tokenizeNoSpecial(const std::string& text) const {
    std::vector<int> ids;
    for (const auto& pt : preTokenize(normalize(text))) {
        for (int id : wordpieceToken(pt)) ids.push_back(id);
    }
    return ids;
}

std::vector<int> WordPieceTokenizer::encode(const std::string& text, int max_length) const {
    auto tokens = tokenizeNoSpecial(text);
    if (max_length > 0) {
        int room = max_length - 2;  // 给 [CLS]/[SEP] 留位，与 HF truncation 一致
        if (room < 0) room = 0;
        if (static_cast<int>(tokens.size()) > room) tokens.resize(room);
    }
    std::vector<int> ids;
    ids.reserve(tokens.size() + 2);
    ids.push_back(cls_id_);
    for (int id : tokens) ids.push_back(id);
    ids.push_back(sep_id_);
    return ids;
}
