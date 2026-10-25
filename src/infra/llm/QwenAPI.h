#pragma once

// QwenAPI - Qwen模型API客户端

#include <string>
#include <vector>

// 多模态内容支持
struct QwenContent {
    std::string type;   // "text"、"image_url" 或 "audio_url"
    std::string text;   // 文本内容（当type为"text"时）
    std::string image_url;  // Base64编码的图像数据URL（当type为"image_url"时）
    std::string audio_url;  // Base64编码的音频数据URL（当type为"audio_url"时）
};

struct QwenMessage {
    std::string role;           // "user", "assistant", "system"
    std::string content;        // 纯文本内容
    std::vector<QwenContent> contents; // 多模态内容（可选）
};

struct QwenChatCompletionRequest {
    std::string model;
    std::vector<QwenMessage> messages;
    float temperature = 0.8;
    float top_p = 0.9;
    int max_tokens = 16384;
};

struct QwenChatCompletionResponse {
    std::string content;
};

struct QwenEmbeddingRequest {
    std::string model;
    std::vector<std::string> input;
};

struct QwenEmbeddingData {
    int index;
    std::vector<float> embedding;
};

struct QwenEmbeddingResponse {
    std::vector<QwenEmbeddingData> data;
    std::string message; // 错误消息
};

class QwenAPI {
public:
    QwenAPI();
    ~QwenAPI();
    
    void set_api_key(const std::string& api_key);
    void set_base_url(const std::string& base_url);
    // Q9：非空时优先于 config.ini 的 api.local_url——让同一进程能同时调两个本地
    // llama-server 实例（文本 LLM :8099 与 VLM :8098）。座舱多模态轮次靠它。
    void set_local_url_override(const std::string& url);

    QwenChatCompletionResponse chat_completion(const QwenChatCompletionRequest& request);
    QwenEmbeddingResponse generate_embeddings(const QwenEmbeddingRequest& request);

private:
    std::string api_key_;
    std::string base_url_;
    std::string local_url_override_;   // Q9：见 set_local_url_override
    bool use_local_api_;
    
    // 辅助方法，用于判断是否使用本地API
    bool shouldUseLocalAPI() const;
};