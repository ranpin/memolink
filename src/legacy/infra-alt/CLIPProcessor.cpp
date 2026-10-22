#include <algorithm>
#include "legacy/infra-alt/CLIPProcessor.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstring>

CLIPProcessor::CLIPProcessor() : embedding_dimension_(512) {
    std::cout << "[CLIPProcessor] 初始化CLIP处理器..." << std::endl;
    std::cout << "[CLIPProcessor] CLIP处理器初始化完成（简化版）" << std::endl;
}

CLIPProcessor::~CLIPProcessor() {
    // 析构函数
}

int CLIPProcessor::getEmbeddingDimension() const {
    return embedding_dimension_;
}

std::vector<float> CLIPProcessor::encodeText(const std::string& text) {
    std::cout << "[CLIPProcessor] 文本编码开始..." << std::endl;
    std::cout << "[CLIPProcessor] 输入文本长度: " << text.length() << " 字符" << std::endl;
    
    // 转义文本中的双引号
    std::string escapedText = text;
    size_t pos = 0;
    while ((pos = escapedText.find("\"", pos)) != std::string::npos) {
        escapedText.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // 构造命令
    std::string command = "cd /Users/ranpin/code/mirix-cpp && /Users/ranpin/code/MIRIX/mirix_env/bin/python scripts/clip_processor.py text \"" + 
                         escapedText + "\" 2>/dev/null";
    
    std::cout << "[CLIPProcessor] 执行命令: " << command.substr(0, 60) << "..." << std::endl;
    
    // 执行Python脚本
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "[CLIPProcessor] 错误: 无法执行Python脚本" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法执行Python脚本");
    }
    
    // 读取结果
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        result += buffer;
    }
    
    // 检查Python脚本是否执行成功
    int exitCode = pclose(pipe.release());
    if (exitCode != 0) {
        std::cerr << "[CLIPProcessor] Python脚本执行失败，退出码: " << exitCode << std::endl;
        std::cerr << "[CLIPProcessor] 返回结果: " << result << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("Python脚本执行失败");
    }
    
    std::cout << "[CLIPProcessor] Python脚本执行成功" << std::endl;
    
    // 简单解析JSON结果（实际项目中应该使用JSON库）
    // 查找embedding数组
    size_t embeddingPos = result.find("\"embedding\":");
    if (embeddingPos == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法找到embedding字段" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法找到embedding字段");
    }
    
    size_t arrayStart = result.find('[', embeddingPos);
    size_t arrayEnd = result.find(']', embeddingPos);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法解析embedding数组" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法解析embedding数组");
    }
    
    std::string arrayContent = result.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // 解析数组内容
    std::vector<float> embedding;
    std::stringstream ss(arrayContent);
    std::string value;
    
    while (std::getline(ss, value, ',')) {
        // 去除空格
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        try {
            embedding.push_back(std::stof(value));
        } catch (const std::exception& e) {
            std::cerr << "[CLIPProcessor] 解析数值时出错: " << value << std::endl;
            // 不直接返回模拟实现，而是抛出异常让调用者处理
            throw std::runtime_error("解析数值时出错");
        }
    }
    
    std::cout << "[CLIPProcessor] 文本编码完成，维度: " << embedding.size() << std::endl;
    return embedding;
}

std::vector<float> CLIPProcessor::encodeImage(const std::string& imagePath) {
    std::cout << "[CLIPProcessor] 图像编码开始..." << std::endl;
    std::cout << "[CLIPProcessor] 图像路径: " << imagePath << std::endl;
    
    // 转义路径中的双引号
    std::string escapedPath = imagePath;
    size_t pos = 0;
    while ((pos = escapedPath.find("\"", pos)) != std::string::npos) {
        escapedPath.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // 构造命令
    std::string command = "cd /Users/ranpin/code/mirix-cpp && /Users/ranpin/code/MIRIX/mirix_env/bin/python scripts/clip_processor.py image \"" + 
                         escapedPath + "\" 2>/dev/null";
    
    std::cout << "[CLIPProcessor] 执行命令: " << command.substr(0, 60) << "..." << std::endl;
    
    // 执行Python脚本
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "[CLIPProcessor] 错误: 无法执行Python脚本" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法执行Python脚本");
    }
    
    // 读取结果
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        result += buffer;
    }
    
    // 检查Python脚本是否执行成功
    int exitCode = pclose(pipe.release());
    if (exitCode != 0) {
        std::cerr << "[CLIPProcessor] Python脚本执行失败，退出码: " << exitCode << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("Python脚本执行失败");
    }
    
    std::cout << "[CLIPProcessor] Python脚本执行成功" << std::endl;
    
    // 简单解析JSON结果（实际项目中应该使用JSON库）
    // 查找embedding数组
    size_t embeddingPos = result.find("\"embedding\":");
    if (embeddingPos == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法找到embedding字段" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法找到embedding字段");
    }
    
    size_t arrayStart = result.find('[', embeddingPos);
    size_t arrayEnd = result.find(']', embeddingPos);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法解析embedding数组" << std::endl;
        // 不直接返回模拟实现，而是抛出异常让调用者处理
        throw std::runtime_error("无法解析embedding数组");
    }
    
    std::string arrayContent = result.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // 解析数组内容
    std::vector<float> embedding;
    std::stringstream ss(arrayContent);
    std::string value;
    
    while (std::getline(ss, value, ',')) {
        // 去除空格
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        try {
            embedding.push_back(std::stof(value));
        } catch (const std::exception& e) {
            std::cerr << "[CLIPProcessor] 解析数值时出错: " << value << std::endl;
            // 不直接返回模拟实现，而是抛出异常让调用者处理
            throw std::runtime_error("解析数值时出错");
        }
    }
    
    std::cout << "[CLIPProcessor] 图像编码完成，维度: " << embedding.size() << std::endl;
    return embedding;
}

std::string CLIPProcessor::generateImageDescription(const std::string& imagePath) {
    std::cout << "[CLIPProcessor] 图像描述生成开始..." << std::endl;
    std::cout << "[CLIPProcessor] 输入图像路径: " << imagePath << std::endl;
    
    // 转义路径中的双引号
    std::string escapedPath = imagePath;
    size_t pos = 0;
    while ((pos = escapedPath.find("\"", pos)) != std::string::npos) {
        escapedPath.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // 构造命令
    std::string command = "cd /Users/ranpin/code/mirix-cpp && /Users/ranpin/code/MIRIX/mirix_env/bin/python scripts/clip_processor.py image_desc \"" + 
                         escapedPath + "\" 2>/dev/null";
    
    std::cout << "[CLIPProcessor] 执行命令: " << command.substr(0, 60) << "..." << std::endl;
    
    // 执行Python脚本
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "[CLIPProcessor] 错误: 无法执行Python脚本" << std::endl;
        throw std::runtime_error("无法执行Python脚本");
    }
    
    // 读取结果
    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        result += buffer;
    }
    
    // 检查Python脚本是否执行成功
    int exitCode = pclose(pipe.release());
    if (exitCode != 0) {
        std::cerr << "[CLIPProcessor] Python脚本执行失败，退出码: " << exitCode << std::endl;
        std::cerr << "[CLIPProcessor] 返回结果: " << result << std::endl;
        throw std::runtime_error("Python脚本执行失败");
    }
    
    std::cout << "[CLIPProcessor] Python脚本执行成功" << std::endl;
    
    // 简单解析JSON结果（实际项目中应该使用JSON库）
    // 查找description字段
    size_t descPos = result.find("\"description\":");
    if (descPos == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法找到description字段" << std::endl;
        throw std::runtime_error("无法找到description字段");
    }
    
    size_t quoteStart = result.find('"', descPos + 15); // 跳过"description":
    size_t quoteEnd = result.find('"', quoteStart + 1);
    if (quoteStart == std::string::npos || quoteEnd == std::string::npos) {
        std::cerr << "[CLIPProcessor] 无法解析description字符串" << std::endl;
        throw std::runtime_error("无法解析description字符串");
    }
    
    std::string description = result.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    
    // 处理转义字符
    size_t escapePos = 0;
    while ((escapePos = description.find("\\\"", escapePos)) != std::string::npos) {
        description.replace(escapePos, 2, "\"");
        escapePos += 1;
    }
    
    std::cout << "[CLIPProcessor] 图像描述生成完成，描述长度: " << description.length() << " 字符" << std::endl;
    return description;
}

std::vector<float> CLIPProcessor::simulateTextEncoding(const std::string& text) {
    std::cout << "[CLIPProcessor] 使用模拟实现生成文本embedding..." << std::endl;
    // 模拟实现：基于文本生成伪随机embedding
    std::vector<float> embedding(512, 0.0f);
    size_t hash = std::hash<std::string>{}(text);
    for (size_t i = 0; i < embedding.size(); ++i) {
        // 使用哈希值生成伪随机数
        size_t value = (hash + i) * 2654435761ULL; // 使用一个大质数
        embedding[i] = static_cast<float>(value % 2000 - 1000) / 1000.0f; // 转换到[-1, 1]范围
    }
    
    // 归一化向量
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm > 0) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    std::cout << "[CLIPProcessor] 模拟文本编码完成，维度: " << embedding.size() << std::endl;
    return embedding;
}

std::vector<float> CLIPProcessor::simulateImageEncoding(const std::string& imagePath) {
    std::cout << "[CLIPProcessor] 使用模拟实现生成图像embedding..." << std::endl;
    // 模拟实现：基于图像路径生成伪随机embedding
    std::vector<float> embedding(512, 0.0f);
    size_t hash = std::hash<std::string>{}(imagePath);
    for (size_t i = 0; i < embedding.size(); ++i) {
        // 使用哈希值生成伪随机数
        size_t value = (hash + i * 3) * 2654435761ULL; // 使用一个大质数
        embedding[i] = static_cast<float>(value % 2000 - 1000) / 1000.0f; // 转换到[-1, 1]范围
    }
    
    // 归一化向量
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    if (norm > 0) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    std::cout << "[CLIPProcessor] 模拟图像编码完成，维度: " << embedding.size() << std::endl;
    return embedding;
}