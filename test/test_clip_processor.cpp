#include <string>
#include "models/CLIPProcessor.h"
#include "capture/MultimodalDataManager.h"
#include "database/initialization/InitDB.h"
#include "utils/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <filesystem>

int main() {
    std::cout << "=== CLIP 处理器测试 ===" << std::endl;
    
    // 数据库文件路径
    std::string db_path = "../database/test_clip.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(db_path)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return 1;
    }
    
    // 初始化存储管理器
    StorageManager storage(db_path);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return 1;
    }
    
    // 创建CLIP处理器
    CLIPProcessor clipProcessor;
    
    // 测试文本编码
    std::cout << "测试文本编码..." << std::endl;
    std::vector<float> textEmbedding = clipProcessor.encodeText("这是一段测试文本");
    std::cout << "文本embedding维度: " << textEmbedding.size() << std::endl;
    if (!textEmbedding.empty()) {
        std::cout << "前5个维度值: ";
        for (int i = 0; i < 5 && i < textEmbedding.size(); ++i) {
            std::cout << textEmbedding[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // 测试图像编码（使用真实图像）
    std::cout << "\n测试图像编码..." << std::endl;
    std::vector<float> imageEmbedding = clipProcessor.encodeImage("test_images/sample_image.png");
    std::cout << "图像embedding维度: " << imageEmbedding.size() << std::endl;
    if (!imageEmbedding.empty()) {
        std::cout << "前5个维度值: ";
        for (int i = 0; i < 5 && i < imageEmbedding.size(); ++i) {
            std::cout << imageEmbedding[i] << " ";
        }
        std::cout << std::endl;
    }
    
    // 测试MultimodalDataManager
    std::cout << "\n测试MultimodalDataManager..." << std::endl;
    MultimodalDataManager dataManager(db_path);
    
    // 生成文本embedding
    std::vector<float> generatedTextEmbedding = dataManager.generateTextEmbedding("测试文本");
    std::cout << "MultimodalDataManager生成的文本embedding维度: " << generatedTextEmbedding.size() << std::endl;
    
    // 生成图像embedding (使用正确的相对路径)
    std::vector<float> generatedImageEmbedding = dataManager.generateImageEmbedding("test_images/sample_image.png");
    std::cout << "MultimodalDataManager生成的图像embedding维度: " << generatedImageEmbedding.size() << std::endl;
    
    // 测试向量序列化
    std::cout << "\n测试向量序列化..." << std::endl;
    std::string serialized = VectorSerializer::serialize(generatedTextEmbedding);
    std::cout << "序列化后数据大小: " << serialized.size() << " 字节" << std::endl;
    
    std::vector<float> deserialized = VectorSerializer::deserialize(serialized);
    std::cout << "反序列化后向量维度: " << deserialized.size() << std::endl;
    
    if (generatedTextEmbedding.size() == deserialized.size()) {
        bool equal = true;
        for (size_t i = 0; i < generatedTextEmbedding.size(); ++i) {
            if (generatedTextEmbedding[i] != deserialized[i]) {
                equal = false;
                break;
            }
        }
        std::cout << "序列化/反序列化正确性: " << (equal ? "通过" : "失败") << std::endl;
    }
    
    // 清理测试数据库
    std::filesystem::remove(db_path);
    
    std::cout << "\n=== 所有测试完成 ===" << std::endl;
    return 0;
}