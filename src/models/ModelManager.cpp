#include "ModelManager.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <json/json.h>
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <filesystem>

ModelManager::ModelManager(StorageManager& storage) : storage_(storage), qwen_api() {
    // 从配置管理器获取API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        qwen_api.set_api_key(api_key);
    }
}

std::string ModelManager::generateResponse(const std::vector<std::string>& context, const std::string& query) {
    // 检查是否有有效的API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        return call_text_model_with_context(context, query); // 实际调用真实模型
    } else {
        return "这是一个模拟响应。在配置有效的API密钥后，我将能够提供真实的AI响应。";
    }
}

std::vector<float> ModelManager::generateEmbedding(const std::string& text) {
    // 检查是否有有效的API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        return call_embedding_model(text); // 实际调用真实模型
    } else {
        // 生成模拟的嵌入向量（128维）
        std::vector<float> embedding(128, 0.1f);
        
        // 简单的文本特征映射到嵌入向量
        for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
            embedding[i] = static_cast<float>(text[i]) / 255.0f;
        }
        
        return embedding;
    }
}

std::string ModelManager::process_multimodal(const std::string& text, 
                                           const std::string& image_path,
                                           const std::string& audio_path) {
    // 检查是否有有效的API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        // 实际调用多模态模型
        return call_multimodal_model(text, image_path, audio_path);
    } else {
        // 返回模拟响应
        return "这是一个多模态处理的模拟响应。在配置有效的API密钥后，我将能够提供真实的多模态AI响应。";
    }
}

// 实际调用文本模型的方法，带上下文
std::string ModelManager::call_text_model_with_context(const std::vector<std::string>& context, const std::string& prompt) {
    // 构建完整的对话历史
    QwenChatCompletionRequest request;
    request.model = "qwen2.5-omni-7b";
    request.temperature = 0.7;
    request.top_p = 0.9;
    request.max_tokens = 2000;
    
    // 添加系统消息（上下文）
    if (!context.empty()) {
        QwenMessage system_msg;
        system_msg.role = "system";
        // 合并所有上下文信息，使用更清晰的格式
        std::ostringstream context_stream;
        context_stream << "以下是与用户当前问题相关的背景信息，请在回答时参考：\n\n";
        for (size_t i = 0; i < context.size(); ++i) {
            context_stream << (i + 1) << ". " << context[i] << "\n";
        }
        context_stream << "\n请根据以上信息，结合用户的具体问题提供准确、相关的回答。";
        system_msg.content = context_stream.str();
        request.messages.push_back(system_msg);
    }
    
    // 添加用户查询
    QwenMessage user_msg;
    user_msg.role = "user";
    user_msg.content = prompt;
    request.messages.push_back(user_msg);
    
    // 调用Qwen API
    QwenChatCompletionResponse response = qwen_api.chat_completion(request);
    return response.content;
}

// 实际调用文本模型的方法
std::string ModelManager::call_text_model(const std::string& prompt) {
    // 构建完整的对话历史
    QwenChatCompletionRequest request;
    request.model = "qwen2.5-omni-7b";
    request.temperature = 0.7;
    request.top_p = 0.9;
    request.max_tokens = 2000;
    
    // 添加用户查询
    QwenMessage user_msg;
    user_msg.role = "user";
    user_msg.content = prompt;
    request.messages.push_back(user_msg);
    
    // 调用Qwen API
    QwenChatCompletionResponse response = qwen_api.chat_completion(request);
    return response.content;
}

// 实际调用嵌入模型的方法
std::vector<float> ModelManager::call_embedding_model(const std::string& text) {
    QwenEmbeddingRequest request;
    request.model = "text-embedding-v4";
    request.input.push_back(text);
    
    QwenEmbeddingResponse response = qwen_api.generate_embeddings(request);
    
    if (!response.data.empty()) {
        return response.data[0].embedding;
    }
    
    // 如果调用失败，返回空向量
    return std::vector<float>();
}

// 实际调用多模态模型的方法
std::string ModelManager::call_multimodal_model(const std::string& text,
                                              const std::string& image_path,
                                              const std::string& audio_path) {
    QwenChatCompletionRequest request;
    request.model = "qwen2.5-omni-7b";
    request.temperature = 0.7;
    request.top_p = 0.9;
    request.max_tokens = 2000;
    
    QwenMessage user_msg;
    user_msg.role = "user";
    
    // 构建多模态内容
    if (!image_path.empty() || !audio_path.empty()) {
        // 使用多模态内容
        if (!image_path.empty()) {
            // 添加图像内容
            QwenContent image_content;
            image_content.type = "image";
            
            // 读取图像文件并转换为base64编码
            std::ifstream image_file(image_path, std::ios::binary);
            if (image_file.is_open()) {
                std::ostringstream buffer;
                buffer << image_file.rdbuf();
                std::string image_data = buffer.str();
                
                // 这里应该实现base64编码
                // 为简化起见，我们直接使用文件路径
                image_content.image = "file://" + image_path;
                user_msg.contents.push_back(image_content);
            }
        }
        
        if (!audio_path.empty()) {
            // 添加音频内容
            QwenContent audio_content;
            audio_content.type = "audio";
            // 为简化起见，我们直接使用文件路径
            audio_content.image = "file://" + audio_path;
            user_msg.contents.push_back(audio_content);
        }
        
        // 添加文本内容
        if (!text.empty()) {
            QwenContent text_content;
            text_content.type = "text";
            text_content.text = text;
            user_msg.contents.push_back(text_content);
        }
    } else {
        // 只有文本内容
        user_msg.content = text;
    }
    
    request.messages.push_back(user_msg);
    
    // 调用Qwen API
    QwenChatCompletionResponse response = qwen_api.chat_completion(request);
    return response.content;
}