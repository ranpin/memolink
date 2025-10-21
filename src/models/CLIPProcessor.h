#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

class CLIPProcessor {
public:
    CLIPProcessor();
    ~CLIPProcessor();
    
    // 生成图像的embedding向量
    std::vector<float> encodeImage(const std::string& imagePath);
    
    // 生成文本的embedding向量
    std::vector<float> encodeText(const std::string& text);
    
    // 获取embedding维度
    int getEmbeddingDimension() const;
    
private:
    int embedding_dimension_;
    
    // 真实的CLIP处理函数
    std::vector<float> generateImageEmbedding(const std::string& imagePath);
    std::vector<float> generateTextEmbedding(const std::string& text);
    
    // 对向量进行归一化
    void normalizeVector(std::vector<float>& vec);
};