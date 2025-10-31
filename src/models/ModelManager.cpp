#include "ModelManager.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <json/json.h>
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <filesystem>
#include <algorithm>

// Base64编码函数
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const std::vector<unsigned char>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    auto it = data.begin();
    while (it != data.end()) {
        char_array_3[i++] = *(it++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

static std::string file_to_base64(const std::string& file_path) {
    std::cout << "[Base64编码] 开始处理文件: " << file_path << std::endl;
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Base64编码] 无法打开文件: " << file_path << std::endl;
        throw std::runtime_error("无法打开文件: " + file_path);
    }
    
    std::cout << "[Base64编码] 文件打开成功" << std::endl;
    
    // 读取文件内容
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();
    
    std::cout << "[Base64编码] 文件读取成功，大小: " << buffer.size() << " 字节" << std::endl;
    
    if (buffer.empty()) {
        std::cerr << "[Base64编码] 文件为空: " << file_path << std::endl;
        throw std::runtime_error("文件为空: " + file_path);
    }
    
    // 进行Base64编码
    std::string base64_data = base64_encode(buffer);
    std::cout << "[Base64编码] 编码完成，编码后长度: " << base64_data.length() << std::endl;
    
    // 打印Base64编码数据的前100个字符用于调试
    if (base64_data.length() > 100) {
        std::cout << "[Base64编码] 编码数据(前100字符): " << base64_data.substr(0, 100) << "..." << std::endl;
    } else {
        std::cout << "[Base64编码] 编码数据: " << base64_data << std::endl;
    }
    
    return base64_data;
}

ModelManager::ModelManager(StorageManager& storage) : storage_(storage), qwen_api() {
    // 从配置管理器获取API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        qwen_api.set_api_key(api_key);
    }
}

std::string ModelManager::generateResponse(const std::vector<std::string>& context, const std::string& query) {
    // 检查是否有有效的API密钥或配置了本地API URL
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    std::string local_api_url = config_manager.getLocalApiUrl();
    
    if (!api_key.empty() || !local_api_url.empty()) {
        return call_text_model_with_context(context, query); // 实际调用真实模型
    } else {
        return "这是一个模拟响应。在配置有效的API密钥后，我将能够提供真实的AI响应。";
    }
}

std::vector<float> ModelManager::generateEmbedding(const std::string& text) {
    // 检查是否有有效的API密钥或配置了本地API URL
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    std::string local_api_url = config_manager.getLocalApiUrl();
    
    if (!api_key.empty() || !local_api_url.empty()) {
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
    // 检查是否有有效的API密钥或配置了本地API URL
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    std::string local_api_url = config_manager.getLocalApiUrl();
    
    if (!api_key.empty() || !local_api_url.empty()) {
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
    
    // 从配置文件中读取模型名称
    ConfigManager& config_manager = ConfigManager::getInstance();
    request.model = config_manager.getTextModelName();
    
    request.temperature = 0.7;
    request.top_p = 0.9;
    request.max_tokens = 2000;  // 修复：将32768改为2000，确保在允许范围内[10, 2048]
    
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
    // 使用配置文件中的模型名称
    request.model = ConfigManager::getInstance().getMultimodalModelName();
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
    std::cout << "\n=== 多模态模型调用开始 ===" << std::endl;
    // 使用配置文件中的模型名称
    std::string model_name = ConfigManager::getInstance().getMultimodalModelName();
    std::cout << "使用模型: " << model_name << std::endl;
    std::cout << "文本输入: " << text << std::endl;
    std::cout << "图片路径: " << image_path << std::endl;
    std::cout << "音频路径: " << audio_path << std::endl;
    
    // 检查文件大小，如果文件存在但超过限制则直接返回错误信息
    if (!image_path.empty()) {
        std::ifstream image_file(image_path, std::ios::binary | std::ios::ate);
        if (image_file.is_open()) {
            size_t image_size = image_file.tellg();
            image_file.close();
            
            if (image_size > 10 * 1024 * 1024) {  // 10MB限制
                std::string error_msg = "图像文件过大(" + std::to_string(image_size) + "字节)，超过10MB限制，无法处理";
                std::cerr << error_msg << std::endl;
                return error_msg;
            }
        } else {
            std::string error_msg = "无法打开图像文件: " + image_path;
            std::cerr << error_msg << std::endl;
            return error_msg;
        }
    }
    
    if (!audio_path.empty()) {
        std::ifstream audio_file(audio_path, std::ios::binary | std::ios::ate);
        if (audio_file.is_open()) {
            size_t audio_size = audio_file.tellg();
            audio_file.close();
            
            if (audio_size > 10 * 1024 * 1024) {  // 10MB限制
                std::string error_msg = "音频文件过大(" + std::to_string(audio_size) + "字节)，超过10MB限制，无法处理";
                std::cerr << error_msg << std::endl;
                return error_msg;
            }
        } else {
            std::string error_msg = "无法打开音频文件: " + audio_path;
            std::cerr << error_msg << std::endl;
            return error_msg;
        }
    }

    QwenChatCompletionRequest request;
    request.model = model_name;
    request.temperature = 0.7;
    request.top_p = 0.9;
    request.max_tokens = 2000;  // 修复：将16384改为2000，确保在允许范围内[10, 2048]
    
    QwenMessage user_msg;
    user_msg.role = "user";
    
    // 构建多模态内容
    if (!image_path.empty() || !audio_path.empty()) {
        std::cout << "构建多模态内容..." << std::endl;
        
        // 使用多模态内容
        if (!image_path.empty()) {
            // 添加图像内容
            try {
                std::cout << "处理图像文件: " << image_path << std::endl;
                QwenContent image_content;
                image_content.type = "image_url";
                
                // 读取图像文件并转换为base64编码
                std::string base64_data = file_to_base64(image_path);
                // 根据文件扩展名确定MIME类型
                std::string mime_type = "image/jpeg";
                if (image_path.find(".png") != std::string::npos || image_path.find(".PNG") != std::string::npos) {
                    mime_type = "image/png";
                } else if (image_path.find(".gif") != std::string::npos || image_path.find(".GIF") != std::string::npos) {
                    mime_type = "image/gif";
                } else if (image_path.find(".webp") != std::string::npos || image_path.find(".WEBP") != std::string::npos) {
                    mime_type = "image/webp";
                }
                image_content.image_url = "data:" + mime_type + ";base64," + base64_data;
                user_msg.contents.push_back(image_content);
                std::cout << "图像内容添加成功" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "图像文件base64编码失败: " << image_path << ", 错误: " << e.what() << std::endl;
                // 即使图像处理失败，也要继续处理文本
            }
        }
        
        if (!audio_path.empty()) {
            // 添加音频内容
            try {
                std::cout << "处理音频文件: " << audio_path << std::endl;
                QwenContent audio_content;
                audio_content.type = "audio_url";
                // 读取音频文件并转换为base64编码
                std::string base64_data = file_to_base64(audio_path);
                audio_content.audio_url = "data:audio/wav;base64," + base64_data;
                user_msg.contents.push_back(audio_content);
                std::cout << "音频内容添加成功" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "音频文件base64编码失败: " << audio_path << ", 错误: " << e.what() << std::endl;
            }
        }
        
        // 检查多模态内容数量，避免超过模型限制
        if (user_msg.contents.size() > 10) {  // 限制最多10个多模态内容项
            std::string warning_msg = "警告: 多模态内容数量(" + std::to_string(user_msg.contents.size()) + ")超过限制(10)，可能会导致API调用失败";
            std::cerr << warning_msg << std::endl;
            // 超过限制时，只保留前10个内容项
            user_msg.contents.resize(10);
            std::cout << "已将多模态内容数量限制为10个" << std::endl;
        }
        
        // 添加文本内容
        if (!text.empty()) {
            std::cout << "添加文本内容: " << text << std::endl;
            QwenContent text_content;
            text_content.type = "text";
            text_content.text = text;
            user_msg.contents.push_back(text_content);
        }
    } else {
        // 只有文本内容
        std::cout << "只有文本内容: " << text << std::endl;
        user_msg.content = text;
    }
    
    request.messages.push_back(user_msg);
    
    // 打印完整的请求信息用于调试
    std::cout << "请求消息数量: " << request.messages.size() << std::endl;
    for (size_t i = 0; i < request.messages.size(); ++i) {
        const auto& msg = request.messages[i];
        std::cout << "消息 " << i << " - 角色: " << msg.role << std::endl;
        if (!msg.content.empty()) {
            std::cout << "  纯文本内容: " << msg.content << std::endl;
        }
        if (!msg.contents.empty()) {
            std::cout << "  多模态内容数量: " << msg.contents.size() << std::endl;
            for (size_t j = 0; j < msg.contents.size(); ++j) {
                const auto& content = msg.contents[j];
                std::cout << "    内容 " << j << " - 类型: " << content.type << std::endl;
                if (content.type == "text" && !content.text.empty()) {
                    std::cout << "      文本: " << content.text << std::endl;
                } else if (content.type == "image_url" && !content.image_url.empty()) {
                    std::cout << "      图像URL长度: " << content.image_url.length() << std::endl;
                    if (content.image_url.length() > 50) {
                        std::cout << "      图像URL(前50字符): " << content.image_url.substr(0, 50) << "..." << std::endl;
                    } else {
                        std::cout << "      图像URL: " << content.image_url << std::endl;
                    }
                } else if (content.type == "audio_url" && !content.audio_url.empty()) {
                    std::cout << "      音频URL长度: " << content.audio_url.length() << std::endl;
                    if (content.audio_url.length() > 50) {
                        std::cout << "      音频URL(前50字符): " << content.audio_url.substr(0, 50) << "..." << std::endl;
                    } else {
                        std::cout << "      音频URL: " << content.audio_url << std::endl;
                    }
                }
            }
        }
    }
    
    std::cout << "=== 调用Qwen API ===" << std::endl;
    
    // 调用Qwen API
    QwenChatCompletionResponse response = qwen_api.chat_completion(request);
    
    std::cout << "=== 多模态模型调用结束 ===" << std::endl;
    
    return response.content;
}