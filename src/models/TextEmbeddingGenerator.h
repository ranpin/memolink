#pragma once

#include "../database/core/StorageManager.h"
#include "CLIPProcessor.h"
#include "ModelManager.h"
#include <string>
#include <vector>
#include <memory>

class TextEmbeddingGenerator {
private:
    StorageManager* storage_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;
    int embedding_dimension_;

public:
    // 构造函数，接受StorageManager指针（可为空）
    explicit TextEmbeddingGenerator(StorageManager* storage);
    
    // 构造函数，接受StorageManager指针和CLIP模型路径
    TextEmbeddingGenerator(StorageManager* storage, const std::string& clip_model_path);
    
    // 生成文本的嵌入向量
    std::vector<float> generateTextEmbedding(const std::string& text);
    
    // 生成文本的嵌入向量（使用传入的ModelManager实例）
    std::vector<float> generateTextEmbedding(const std::string& text, ModelManager& modelManager);
    
    // 生成图像描述
    std::string generateImageDescription(const std::string& image_path);
    
    // 生成图像描述（使用传入的ModelManager实例）
    std::string generateImageDescription(const std::string& image_path, ModelManager& modelManager);
    
    // 获取嵌入向量的维度
    int getEmbeddingDimension() const;
    
    // 兼容旧版本的方法名
    std::vector<float> generateEmbedding(const std::string& text) {
        return generateTextEmbedding(text);
    }
    
    std::vector<float> generateEmbedding(const std::string& text, ModelManager& modelManager) {
        return generateTextEmbedding(text, modelManager);
    }
};