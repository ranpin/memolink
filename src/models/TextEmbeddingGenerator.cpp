#include "TextEmbeddingGenerator.h"
#include "ModelManager.h"
#include "CLIPProcessor.h"
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <iostream>
#include <cmath>

TextEmbeddingGenerator::TextEmbeddingGenerator() : embedding_dimension_(512) {
    std::cout << "[TextEmbeddingGenerator] 初始化..." << std::endl;
    
    // 使用CLIP模型维度（512维）作为默认值
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string embedding_model = config_manager.getEmbeddingModelName();
    
    std::cout << "[TextEmbeddingGenerator] 配置的模型: " << embedding_model << std::endl;
    
    // 根据配置的模型名称设置维度
    // 注意：这里应该从配置或模型API获取实际维度，而不是硬编码
    if (embedding_model == "text-embedding-v4") {
        // text-embedding-v4的实际维度是1536，不是128
        embedding_dimension_ = 1536;
        std::cout << "[TextEmbeddingGenerator] 设置维度为1536 (text-embedding-v4)" << std::endl;
    } else if (embedding_model == "CLIP") {
        // CLIP模型维度是512
        embedding_dimension_ = 512;
        std::cout << "[TextEmbeddingGenerator] 设置维度为512 (CLIP)" << std::endl;
    } else {
        // 默认使用CLIP的512维
        embedding_dimension_ = 512;
        std::cout << "[TextEmbeddingGenerator] 使用默认维度512" << std::endl;
    }
    
    // 初始化CLIP处理器
    std::cout << "[TextEmbeddingGenerator] 初始化CLIP处理器..." << std::endl;
    clipProcessor_ = std::make_unique<CLIPProcessor>();
    std::cout << "[TextEmbeddingGenerator] CLIP处理器初始化完成" << std::endl;
    
    std::cout << "[TextEmbeddingGenerator] 初始化完成，配置维度: " << embedding_dimension_ << std::endl;
}

std::vector<float> TextEmbeddingGenerator::generateEmbedding(const std::string& text) {
    std::cout << "[TextEmbeddingGenerator] 开始生成文本embedding..." << std::endl;
    std::cout << "[TextEmbeddingGenerator] 输入文本长度: " << text.length() << " 字符" << std::endl;
    
    // 优先使用CLIP处理器生成文本embedding
    if (clipProcessor_) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用CLIP处理器生成文本embedding..." << std::endl;
        try {
            std::vector<float> clipEmbedding = clipProcessor_->encodeText(text);
            if (!clipEmbedding.empty()) {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器生成文本embedding成功，维度: " << clipEmbedding.size() << std::endl;
                return clipEmbedding;
            } else {
                std::cout << "[TextEmbeddingGenerator] CLIP处理器返回空embedding，尝试其他方法..." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[TextEmbeddingGenerator] CLIP文本编码失败: " << e.what() << std::endl;
        }
    }
    
    // 检查是否有有效的API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        std::cout << "[TextEmbeddingGenerator] 尝试使用text-embedding-v4模型生成embedding..." << std::endl;
        
        // 使用真实的模型生成embedding
        // 创建一个临时的StorageManager实例（仅用于ModelManager初始化）
        StorageManager storage(""); // 空数据库路径，因为我们只调用API
        
        // 创建ModelManager实例
        ModelManager model_manager(storage);
        
        // 调用真实的embedding模型
        std::vector<float> embedding = model_manager.generateEmbedding(text);
        
        // 如果API调用成功并返回了有效的embedding
        if (!embedding.empty()) {
            std::cout << "[TextEmbeddingGenerator] text-embedding-v4模型生成embedding成功，维度: " << embedding.size() << std::endl;
            // 确保维度匹配
            if (embedding.size() != embedding_dimension_) {
                std::cout << "[TextEmbeddingGenerator] 警告: 生成的embedding维度(" << embedding.size() 
                         << ")与配置维度(" << embedding_dimension_ << ")不匹配" << std::endl;
            }
            return embedding;
        }
        
        // 如果API调用失败，回退到模拟实现
        std::cerr << "[TextEmbeddingGenerator] 警告: text-embedding-v4模型调用失败，回退到模拟实现" << std::endl;
    } else {
        std::cout << "[TextEmbeddingGenerator] 未配置API密钥，跳过text-embedding-v4模型" << std::endl;
    }
    
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

int TextEmbeddingGenerator::getEmbeddingDimension() const {
    return embedding_dimension_;
}