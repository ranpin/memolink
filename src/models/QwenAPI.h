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
    std::string model = "qwen-plus";
    std::vector<QwenMessage> messages;
    float temperature = 0.8;
    float top_p = 0.9;
    int max_tokens = 16384;
    float top_k = 0; // 为0表示不使用top_k
    float repetition_penalty = 1.0;
    std::string stop; // 停止词
    bool stream = false;
    
    // 验证请求参数有效性
    bool validate(std::string* error_message = nullptr) const {
        if (model.empty()) {
            if (error_message) *error_message = "Model name cannot be empty";
            return false;
        }
        if (messages.empty()) {
            if (error_message) *error_message = "Messages cannot be empty";
            return false;
        }
        if (temperature < 0.0f || temperature > 2.0f) {
            if (error_message) *error_message = "Temperature must be between 0.0 and 2.0";
            return false;
        }
        if (top_p < 0.0f || top_p > 1.0f) {
            if (error_message) *error_message = "Top_p must be between 0.0 and 1.0";
            return false;
        }
        if (max_tokens <= 0) {
            if (error_message) *error_message = "Max_tokens must be greater than 0";
            return false;
        }
        // 验证messages
        for (const auto& msg : messages) {
            if (msg.role.empty()) {
                if (error_message) *error_message = "Message role cannot be empty";
                return false;
            }
            if (msg.role != "user" && msg.role != "assistant" && msg.role != "system") {
                if (error_message) *error_message = "Invalid message role: " + msg.role;
                return false;
            }
            if (msg.content.empty() && msg.contents.empty()) {
                if (error_message) *error_message = "Message content cannot be empty";
                return false;
            }
        }
        return true;
    }
};

struct QwenChatCompletionResponse {
    std::string content;
    bool success = true;        // 标记请求是否成功
    std::string error_message;  // 错误信息
    
    // 验证响应有效性
    bool validate(std::string* error_msg = nullptr) const {
        if (!success) {
            if (error_msg) *error_msg = error_message.empty() ? "Request failed" : error_message;
            return false;
        }
        if (content.empty()) {
            if (error_msg) *error_msg = "Response content is empty";
            return false;
        }
        return true;
    }
};

struct QwenEmbeddingRequest {
    std::string model = "text-embedding-v3";
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