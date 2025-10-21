#include "TextEmbeddingGenerator.h"
#include "ModelManager.h"
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <iostream>
#include <cmath>

TextEmbeddingGenerator::TextEmbeddingGenerator() : embedding_dimension_(128) {
    // 可以根据配置调整维度
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string embedding_model = config_manager.getEmbeddingModelName();
    
    // 根据模型名称设置维度
    if (embedding_model == "text-embedding-v4") {
        embedding_dimension_ = 128; // Qwen的文本嵌入维度
    } else {
        embedding_dimension_ = 128; // 默认维度
    }
}

std::vector<float> TextEmbeddingGenerator::generateEmbedding(const std::string& text) {
    // 检查是否有有效的API密钥
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    
    if (!api_key.empty()) {
        // 使用真实的模型生成embedding
        // 创建一个临时的StorageManager实例（仅用于ModelManager初始化）
        StorageManager storage(""); // 空数据库路径，因为我们只调用API
        
        // 创建ModelManager实例
        ModelManager model_manager(storage);
        
        // 调用真实的embedding模型
        std::vector<float> embedding = model_manager.generateEmbedding(text);
        
        // 如果API调用成功并返回了有效的embedding
        if (!embedding.empty()) {
            return embedding;
        }
        
        // 如果API调用失败，回退到模拟实现
        std::cerr << "警告: 真实模型调用失败，回退到模拟实现" << std::endl;
    }
    
    // 生成模拟的嵌入向量
    std::vector<float> embedding(embedding_dimension_, 0.0f);
    
    // 简单的文本特征映射到嵌入向量
    for (size_t i = 0; i < text.length() && i < embedding.size(); ++i) {
        embedding[i] = std::tanh(static_cast<float>(text[i]) / 128.0f); // 使用tanh归一化
    }
    
    return embedding;
}

int TextEmbeddingGenerator::getEmbeddingDimension() const {
    return embedding_dimension_;
}