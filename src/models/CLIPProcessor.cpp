#include "CLIPProcessor.h"
#include <iostream>
#include <cmath>
#include <random>
#include <filesystem>

CLIPProcessor::CLIPProcessor() : embedding_dimension_(512) {
    // CLIP模型通常输出512维向量
    // 简化实现，避免复杂的模型加载
    std::cout << "CLIP处理器初始化完成（简化版）" << std::endl;
}

CLIPProcessor::~CLIPProcessor() = default;

std::vector<float> CLIPProcessor::encodeImage(const std::string& imagePath) {
    // 检查图像文件是否存在
    if (imagePath.empty()) {
        std::cerr << "错误: 图像路径为空" << std::endl;
        return std::vector<float>(embedding_dimension_, 0.0f);
    }
    
    // 生成基于图像内容的伪embedding（简化版真实实现）
    std::vector<float> embedding = generateImageEmbedding(imagePath);
    
    // 如果生成失败，回退到模拟实现
    if (embedding.empty()) {
        std::cerr << "使用模拟实现生成图像embedding" << std::endl;
        embedding = std::vector<float>(embedding_dimension_, 0.0f);
        
        // 使用文件路径的哈希值作为特征
        std::hash<std::string> hasher;
        size_t hash = hasher(imagePath);
        
        // 使用哈希值生成伪随机embedding
        std::mt19937 gen(static_cast<unsigned int>(hash));
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        for (int i = 0; i < embedding_dimension_; ++i) {
            embedding[i] = dis(gen);
        }
        
        // 归一化向量
        normalizeVector(embedding);
    }
    
    return embedding;
}

std::vector<float> CLIPProcessor::encodeText(const std::string& text) {
    // 生成基于文本内容的伪embedding（简化版真实实现）
    std::vector<float> embedding = generateTextEmbedding(text);
    
    // 如果生成失败，回退到模拟实现
    if (embedding.empty()) {
        std::cerr << "使用模拟实现生成文本embedding" << std::endl;
        embedding = std::vector<float>(embedding_dimension_, 0.0f);
        
        // 使用文本的哈希值作为特征
        std::hash<std::string> hasher;
        size_t hash = hasher(text);
        
        // 使用哈希值生成伪随机embedding
        std::mt19937 gen(static_cast<unsigned int>(hash));
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        for (int i = 0; i < embedding_dimension_; ++i) {
            embedding[i] = dis(gen);
        }
        
        // 归一化向量
        normalizeVector(embedding);
    }
    
    return embedding;
}

int CLIPProcessor::getEmbeddingDimension() const {
    return embedding_dimension_;
}

std::vector<float> CLIPProcessor::generateImageEmbedding(const std::string& imagePath) {
    try {
        // 检查文件是否存在
        if (!std::filesystem::exists(imagePath)) {
            std::cerr << "图像文件不存在: " << imagePath << std::endl;
            return std::vector<float>();
        }
        
        // 尝试加载图像
        cv::Mat image = cv::imread(imagePath);
        if (image.empty()) {
            std::cerr << "无法加载图像: " << imagePath << std::endl;
            return std::vector<float>();
        }
        
        // 简化的图像特征提取
        // 计算图像的一些基本统计特征作为embedding的基础
        std::vector<float> features;
        
        // 计算平均颜色值
        cv::Scalar meanColor = cv::mean(image);
        features.push_back(static_cast<float>(meanColor[0]) / 255.0f); // Blue
        features.push_back(static_cast<float>(meanColor[1]) / 255.0f); // Green
        features.push_back(static_cast<float>(meanColor[2]) / 255.0f); // Red
        
        // 计算图像梯度
        cv::Mat gray, gradX, gradY;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::Sobel(gray, gradX, CV_32F, 1, 0);
        cv::Sobel(gray, gradY, CV_32F, 0, 1);
        
        cv::Scalar meanGradX = cv::mean(gradX);
        cv::Scalar meanGradY = cv::mean(gradY);
        features.push_back(static_cast<float>(meanGradX[0]) / 255.0f);
        features.push_back(static_cast<float>(meanGradY[0]) / 255.0f);
        
        // 生成完整的embedding向量
        std::vector<float> embedding(embedding_dimension_, 0.0f);
        
        // 使用特征值填充embedding的一部分
        for (size_t i = 0; i < std::min(features.size(), static_cast<size_t>(embedding_dimension_)); ++i) {
            embedding[i] = features[i];
        }
        
        // 使用文件路径的哈希值填充剩余部分
        std::hash<std::string> hasher;
        size_t hash = hasher(imagePath);
        std::mt19937 gen(static_cast<unsigned int>(hash));
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        for (int i = static_cast<int>(features.size()); i < embedding_dimension_; ++i) {
            embedding[i] = dis(gen);
        }
        
        // 归一化向量
        normalizeVector(embedding);
        
        return embedding;
    } catch (const std::exception& e) {
        std::cerr << "图像编码异常: " << e.what() << std::endl;
        return std::vector<float>();
    }
}

std::vector<float> CLIPProcessor::generateTextEmbedding(const std::string& text) {
    try {
        if (text.empty()) {
            std::cerr << "文本为空" << std::endl;
            return std::vector<float>();
        }
        
        // 简化的文本特征提取
        std::vector<float> features;
        
        // 文本长度特征
        features.push_back(static_cast<float>(text.length()) / 100.0f);
        
        // 字符频率特征
        int vowelCount = 0;
        int consonantCount = 0;
        int digitCount = 0;
        int spaceCount = 0;
        
        for (char c : text) {
            if (std::isalpha(c)) {
                char lower = std::tolower(c);
                if (lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u') {
                    vowelCount++;
                } else {
                    consonantCount++;
                }
            } else if (std::isdigit(c)) {
                digitCount++;
            } else if (std::isspace(c)) {
                spaceCount++;
            }
        }
        
        features.push_back(static_cast<float>(vowelCount) / text.length());
        features.push_back(static_cast<float>(consonantCount) / text.length());
        features.push_back(static_cast<float>(digitCount) / text.length());
        features.push_back(static_cast<float>(spaceCount) / text.length());
        
        // 生成完整的embedding向量
        std::vector<float> embedding(embedding_dimension_, 0.0f);
        
        // 使用特征值填充embedding的一部分
        for (size_t i = 0; i < std::min(features.size(), static_cast<size_t>(embedding_dimension_)); ++i) {
            embedding[i] = features[i];
        }
        
        // 使用文本的哈希值填充剩余部分
        std::hash<std::string> hasher;
        size_t hash = hasher(text);
        std::mt19937 gen(static_cast<unsigned int>(hash));
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        for (int i = static_cast<int>(features.size()); i < embedding_dimension_; ++i) {
            embedding[i] = dis(gen);
        }
        
        // 归一化向量
        normalizeVector(embedding);
        
        return embedding;
    } catch (const std::exception& e) {
        std::cerr << "文本编码异常: " << e.what() << std::endl;
        return std::vector<float>();
    }
}


void CLIPProcessor::normalizeVector(std::vector<float>& vec) {
    // 计算向量的模长
    float norm = 0.0f;
    for (float val : vec) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    // 归一化向量
    if (norm > 0) {
        for (float& val : vec) {
            val /= norm;
        }
    }
}