#pragma once

// QwenAPI - Qwen模型API客户端

#include <string>
#include <vector>

// 多模态内容支持
struct QwenContent {
    std::string type;   // "text"、"image_url" 或 "audio_url"
    std::string text;   // 文本内容（当type为"text"时）
    std::string image_url;  // Base64编码的图像数据或图像URL（当type为"image_url"时）
    std::string audio_url;  // Base64编码的音频数据或音频URL（当type为"audio_url"时）
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
    int max_tokens = 1500;
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
    
    QwenChatCompletionResponse chat_completion(const QwenChatCompletionRequest& request);
    QwenEmbeddingResponse generate_embeddings(const QwenEmbeddingRequest& request);

private:
    std::string api_key_;
    std::string base_url_;
    bool use_local_api_;
    
    // 辅助方法，用于判断是否使用本地API
    bool shouldUseLocalAPI() const;
};