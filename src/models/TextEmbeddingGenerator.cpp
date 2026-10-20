#include "TextEmbeddingGenerator.h"
#include "QwenAPI.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "../database/initialization/ConfigManager/ConfigManager.h"

TextEmbeddingGenerator::TextEmbeddingGenerator(StorageManager* storage) : storage_(storage) {
    std::cout << "[TextEmbeddingGenerator] 初始化..." << std::endl;
    
    // 获取配置管理器实例
    ConfigManager& config_manager = ConfigManager::getInstance();
    
    // 获取配置的模型和维度
    std::string model_name = config_manager.getEmbeddingModelName();
    
    // 根据模型名称设置维度
    if (model_name == "text-embedding-v4") {
        embedding_dimension_ = 1024; // text-embedding-v4模型维度
    } else if (model_name == "text-embedding-v1") {
        embedding_dimension_ = 1536; // text-embedding-v1模型维度
    } else {
        embedding_dimension_ = 128; // 默认维度
    }
    
    std::cout << "[TextEmbeddingGenerator] 配置的模型: " << model_name << std::endl;
    std::cout << "[TextEmbeddingGenerator] 设置维度为" << embedding_dimension_ << " (" << model_name << ")" << std::endl;
    
#ifdef MIRIX_HAS_CLIP
    // 初始化CLIP处理器（仅桌面多模态构建）
    std::cout << "[TextEmbeddingGenerator] 初始化CLIP处理器..." << std::endl;
    clipProcessor_ = std::make_unique<CLIPProcessor>();
    std::cout << "[TextEmbeddingGenerator] CLIP处理器初始化完成" << std::endl;
#endif
    
    std::cout << "[TextEmbeddingGenerator] 初始化完成，配置维度: " << embedding_dimension_ << std::endl;
}

std::vector<float> TextEmbeddingGenerator::generateTextEmbedding(const std::string& text) {
    std::cout << "[TextEmbeddingGenerator] 开始生成文本embedding..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 输入文本长度: " << text.length() << " 字符" << std::endl;
    
    // 优先使用text-embedding-v4模型生成文本embedding（通过Qwen API）
    const char* api_key = std::getenv("DASHSCOPE_API_KEY");
    if (api_key && strlen(api_key) > 0) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用text-embedding-v4模型生成文本embedding..." << std::endl;
        try {
            // 调用Qwen API的text-embedding-v4模型
            QwenAPI qwen_api;
            qwen_api.set_api_key(api_key);
            QwenEmbeddingRequest request;
            request.model = "text-embedding-v4";
            request.input.push_back(text);
            QwenEmbeddingResponse response = qwen_api.generate_embeddings(request);
            
            if (!response.data.empty()) {
                std::vector<float> embedding = response.data[0].embedding;
                // 确保维度匹配
                if (embedding.size() != embedding_dimension_) {
                    std::cout << "[TextEmbeddingGenerator] 警告: 生成的embedding维度(" << embedding.size() 
                             << ")与配置维度(" << embedding_dimension_ << ")不匹配" << std::endl;
                }
                std::cout << "[TextEmbeddingGenerator] text-embedding-v4模型生成文本embedding成功，维度: " << embedding.size() << std::endl;
                return embedding;
            } else {
                throw std::runtime_error("Empty embedding response");
            }
        }
        
        // 如果API调用失败，回退到模拟实现
        catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] 警告: text-embedding-v4模型调用失败: " << e.what() << std::endl;
            std::cerr << "[TextEmbeddingGenerator] 回退到CLIP处理器或模拟实现" << std::endl;
        }
    } else {
        std::cout << "[TextEmbeddingGenerator] 未配置API密钥，跳过text-embedding-v4模型" << std::endl;
    }
    
#ifdef MIRIX_HAS_CLIP
    // 备选用CLIP处理器生成文本embedding（仅桌面多模态构建）
    if (clipProcessor_) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用CLIP处理器生成文本embedding..." << std::endl;
        try {
            std::vector<float> clipEmbedding = clipProcessor_->encodeText(text);
            if (!clipEmbedding.empty()) {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器生成文本embedding成功，维度: " << clipEmbedding.size() << std::endl;
                return clipEmbedding;
            } else {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器返回空embedding" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] CLIP文本编码失败: " << e.what() << std::endl;
        }
    }
#endif
    
    // 生成模拟的嵌入向量
    std::cout << "[TextEmbeddingGenerator] 使用模拟实现生成embedding，维度: " << embedding_dimension_ << std::endl;
    std::vector<float> embedding(embedding_dimension_, 0.0f);
    
    // 简单的文本特征映射到嵌入向量
    for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
        embedding[i] = std::tanh(static_cast<float>(text[i]) / 128.0f); // 使用tanh归一化
    }
    
    std::cout << "[TextEmbeddingGenerator] 模拟embedding生成完成" << std::endl;
    return embedding;
}

// 生成文本的嵌入向量（使用传入的ModelManager实例）
std::vector<float> TextEmbeddingGenerator::generateTextEmbedding(const std::string& text, ModelManager& modelManager) {
    std::cout << "[TextEmbeddingGenerator] 开始生成文本embedding..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 输入文本长度: " << text.length() << " 字符" << std::endl;
    
    // 使用传入的ModelManager实例生成文本embedding
    std::cout << "[TextEmbeddingGenerator] 使用传入的ModelManager实例生成文本embedding..." << std::endl;
    try {
        // 调用模型管理器的嵌入生成方法
        std::vector<float> embedding = modelManager.generateEmbedding(text);
        
        if (!embedding.empty()) {
            // 确保维度匹配
            if (embedding.size() != embedding_dimension_) {
                std::cout << "[TextEmbeddingGenerator] 警告: 生成的embedding维度(" << embedding.size() 
                         << ")与配置维度(" << embedding_dimension_ << ")不匹配" << std::endl;
            }
            std::cout << "[TextEmbeddingGenerator] ModelManager生成文本embedding成功，维度: " << embedding.size() << std::endl;
            return embedding;
        } else {
            throw std::runtime_error("Empty embedding response");
        }
    }
    // 如果API调用失败，回退到模拟实现
    catch (const std::exception& e) {
        std::cerr << "[TextEmbeddingGenerator] 警告: ModelManager调用失败: " << e.what() << std::endl;
        std::cerr << "[TextEmbeddingGenerator] 回退到CLIP处理器或模拟实现" << std::endl;
    }
    
#ifdef MIRIX_HAS_CLIP
    // 备选用CLIP处理器生成文本embedding（仅桌面多模态构建）
    if (clipProcessor_) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用CLIP处理器生成文本embedding..." << std::endl;
        try {
            std::vector<float> clipEmbedding = clipProcessor_->encodeText(text);
            if (!clipEmbedding.empty()) {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器生成文本embedding成功，维度: " << clipEmbedding.size() << std::endl;
                return clipEmbedding;
            } else {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器返回空embedding" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] CLIP文本编码失败: " << e.what() << std::endl;
        }
    }
#endif
    
    // 生成模拟的嵌入向量
    std::cout << "[TextEmbeddingGenerator] 使用模拟实现生成embedding，维度: " << embedding_dimension_ << std::endl;
    std::vector<float> embedding(embedding_dimension_, 0.0f);
    
    // 简单的文本特征映射到嵌入向量
    for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
        embedding[i] = std::tanh(static_cast<float>(text[i]) / 128.0f); // 使用tanh归一化
    }
    
    std::cout << "[TextEmbeddingGenerator] 模拟embedding生成完成" << std::endl;
    return embedding;
}

std::string TextEmbeddingGenerator::generateImageDescription(const std::string& image_path) {
    std::cout << "[TextEmbeddingGenerator] 开始生成图像描述..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 图像路径: " << image_path << std::endl;
    
    // 使用qwen-omni模型生成图像描述（如果可用）
    if (storage_) {
        try {
            std::cout << "[TextEmbeddingGenerator] 使用qwen-omni模型生成图像描述..." << std::endl;
            ModelManager modelManager(*storage_);
            std::string description = modelManager.process_multimodal("请描述这张图片的内容", image_path);
            
            if (!description.empty()) {
                std::cout << "[TextEmbeddingGenerator] qwen-omni模型生成图像描述成功: " << description << std::endl;
                return description;
            } else {
                std::cout << "[TextEmbeddingGenerator] qwen-omni模型返回空描述" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] qwen-omni模型图像描述生成失败: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[TextEmbeddingGenerator] qwen-omni模型图像描述生成出现未知错误" << std::endl;
        }
    } else {
        std::cout << "[TextEmbeddingGenerator] 未提供StorageManager，跳过qwen-omni模型" << std::endl;
    }
    
#ifdef MIRIX_HAS_CLIP
    // 使用CLIP处理器生成图像描述（作为备选方案，仅桌面多模态构建）
    if (clipProcessor_) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用CLIP处理器生成图像描述..." << std::endl;
        try {
            std::string description = clipProcessor_->generateImageDescription(image_path);
            if (!description.empty()) {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器生成图像描述成功: " << description << std::endl;
                return description;
            } else {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器返回空描述" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] CLIP图像描述生成失败: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[TextEmbeddingGenerator] CLIP图像描述生成出现未知错误" << std::endl;
        }
    } else {
        std::cout << "[TextEmbeddingGenerator] CLIP处理器未初始化" << std::endl;
    }
#endif

    // 使用默认图像描述
    std::cout << "[TextEmbeddingGenerator] 使用默认图像描述" << std::endl;
    return "这是一张图片，但未生成具体描述";
}

// 重载版本，接受ModelManager参数
std::string TextEmbeddingGenerator::generateImageDescription(const std::string& image_path, ModelManager& modelManager) {
    std::cout << "[TextEmbeddingGenerator] 开始生成图像描述..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 图像路径: " << image_path << std::endl;
    
    // 使用传入的ModelManager实例调用qwen-omni模型生成图像描述
    try {
        std::cout << "[TextEmbeddingGenerator] 使用传入的ModelManager实例调用qwen-omni模型生成图像描述..." << std::endl;
        std::string description = modelManager.process_multimodal("请描述这张图片的内容", image_path);
        
        if (!description.empty()) {
            std::cout << "[TextEmbeddingGenerator] qwen-omni模型生成图像描述成功: " << description << std::endl;
            return description;
        } else {
            std::cout << "[TextEmbeddingGenerator] qwen-omni模型返回空描述" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TextEmbeddingGenerator] qwen-omni模型图像描述生成失败: " << e.what() << std::endl;
    }
    
#ifdef MIRIX_HAS_CLIP
    // 使用CLIP处理器生成图像描述（作为备选方案，仅桌面多模态构建）
    if (clipProcessor_) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用CLIP处理器生成图像描述..." << std::endl;
        try {
            std::string description = clipProcessor_->generateImageDescription(image_path);
            if (!description.empty()) {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器生成图像描述成功: " << description << std::endl;
                return description;
            } else {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器返回空描述" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] CLIP图像描述生成失败: " << e.what() << std::endl;
        }
    }
#endif

    // 如果所有方法都失败，返回默认描述
    std::cout << "[TextEmbeddingGenerator] 使用默认图像描述" << std::endl;
    return "这是一张图片，但未生成具体描述";
}

int TextEmbeddingGenerator::getEmbeddingDimension() const {
    return embedding_dimension_;
}