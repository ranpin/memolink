#pragma once

#include <string>
#include <vector>

class TextEmbeddingGenerator {
public:
    TextEmbeddingGenerator();
    
    // 生成文本的embedding向量
    std::vector<float> generateEmbedding(const std::string& text);
    
    // 获取embedding维度
    int getEmbeddingDimension() const;
    
private:
    int embedding_dimension_;
};