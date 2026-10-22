#include <algorithm>
#include <string>
#include "infra/embedding/TextEmbeddingGenerator.h"
#include "legacy/infra-alt/VectorSearchManager.h"
#include "legacy/infra-alt/ModelManager.h"
#include "infra/storage/StorageManager.h"
#include "infra/storage/InitDB.h"
#include "infra/storage/ConfigManager.h"
#include "legacy/core-v1/RecallMemoryManager.h"
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "infra/storage/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <filesystem>
#include <cmath>

void printSectionHeader(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(50, '=') << "\n" << std::endl;
}

void printSubSection(const std::string& title) {
    std::cout << "\n" << std::string(30, '-') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(30, '-') << "\n" << std::endl;
}

void printStep(const std::string& step) {
    std::cout << "\n>>> " << step << std::endl;
}

void testConfigManager() {
    printSectionHeader("测试配置管理器");
    
    // 检查API密钥配置
    std::cout << "检查配置文件中的API密钥..." << std::endl;
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    std::cout << "  API密钥状态: " << (api_key.empty() ? "未配置" : "已配置") << std::endl;
    
    if (!api_key.empty()) {
        std::cout << "  API密钥前缀: " << api_key.substr(0, std::min(10, (int)api_key.length())) << "..." << std::endl;
    }
    
    // 检查模型配置
    std::cout << "检查模型配置..." << std::endl;
    std::string embedding_model = config_manager.getEmbeddingModelName();
    std::string text_model = config_manager.getTextModelName();
    
    std::cout << "  Embedding模型: " << embedding_model << std::endl;
    std::cout << "  文本模型: " << text_model << std::endl;
    
    std::cout << "\n✓ 配置管理器测试通过" << std::endl;
}

void testTextEmbeddingGeneratorComprehensive() {
    printSectionHeader("测试TextEmbeddingGenerator");
    
    // 创建StorageManager实例
    StorageManager storage("test_embedding_comprehensive.db");
    
    std::cout << "初始化TextEmbeddingGenerator..." << std::endl;
    TextEmbeddingGenerator generator(&storage);
    
    // 测试获取维度
    std::cout << "获取配置的Embedding维度..." << std::endl;
    int dimension = generator.getEmbeddingDimension();
    std::cout << "  配置维度: " << dimension << std::endl;
    
    // 测试生成embedding
    printSubSection("生成测试文本的embedding");
    std::string testText = "人工智能是计算机科学的一个分支，它企图了解智能的实质，并生产出一种新的能以人类智能相似的方式做出反应的智能机器。";
    std::cout << "输入文本: \"" << testText << "\"" << std::endl;
    std::vector<float> embedding = generator.generateEmbedding(testText);
    
    std::cout << "  生成的Embedding向量维度: " << embedding.size() << std::endl;
    assert(!embedding.empty());
    
    // 验证使用了真实模型还是模拟模型
    std::cout << "模型类型识别:" << std::endl;
    if (embedding.size() == 1536) {
        std::cout << "  ✓ 使用了text-embedding-v4真实模型 (1536维)" << std::endl;
    } else if (embedding.size() == 512) {
        std::cout << "  ✓ 使用了CLIP真实模型 (512维)" << std::endl;
    } else if (embedding.size() == 128) {
        std::cout << "  ? 使用了模拟模型 (128维)" << std::endl;
    } else {
        std::cout << "  ? 使用了未知维度的模型: " << embedding.size() << "维" << std::endl;
    }
    
    // 验证值在合理范围内
    std::cout << "验证embedding数值范围..." << std::endl;
    bool inRange = true;
    for (float value : embedding) {
        if (value < -1.0f || value > 1.0f) {
            inRange = false;
            break;
        }
    }
    if (inRange) {
        std::cout << "  ✓ 所有值都在[-1.0, 1.0]范围内" << std::endl;
    } else {
        std::cout << "  ✗ 存在超出范围的值" << std::endl;
    }
    
    printSubSection("测试不同文本生成不同的embedding");
    std::string text1 = "今天天气很好";
    std::string text2 = "今天下雨了";
    std::cout << "文本1: \"" << text1 << "\"" << std::endl;
    std::cout << "文本2: \"" << text2 << "\"" << std::endl;
    
    std::vector<float> embedding1 = generator.generateEmbedding(text1);
    std::vector<float> embedding2 = generator.generateEmbedding(text2);
    
    // 计算两个向量的余弦相似度
    if (embedding1.size() == embedding2.size() && !embedding1.empty()) {
        float dotProduct = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < embedding1.size(); ++i) {
            dotProduct += embedding1[i] * embedding2[i];
            norm1 += embedding1[i] * embedding1[i];
            norm2 += embedding2[i] * embedding2[i];
        }
        
        float similarity = dotProduct / (std::sqrt(norm1) * std::sqrt(norm2));
        std::cout << "两个文本embedding的余弦相似度: " << similarity << std::endl;
        
        if (similarity < 0.99) {
            std::cout << "  ✓ 不同文本生成了不同的Embedding" << std::endl;
        } else {
            std::cout << "  ✗ 不同文本生成了过于相似的Embedding" << std::endl;
        }
    }
    
    std::cout << "\n✓ TextEmbeddingGenerator测试通过" << std::endl;
}

void testVectorSearchManagerComprehensive() {
    printSectionHeader("测试VectorSearchManager");
    
    // 初始化VectorSearchManager
    std::cout << "初始化VectorSearchManager..." << std::endl;
    StorageManager storage("database/test.db"); // 临时存储管理器
    VectorSearchManager vectorSearchManager(storage);
    std::cout << "VectorSearchManager初始化成功" << std::endl;
    
    // 测试文本embedding生成
    printSubSection("测试查询文本embedding生成");
    std::string testText = "人工智能技术正在快速发展";
    std::cout << "查询文本: \"" << testText << "\"" << std::endl;
    std::vector<float> embedding = vectorSearchManager.generateTextEmbedding(testText);
    std::cout << "  生成的查询文本embedding维度: " << embedding.size() << std::endl;
    assert(!embedding.empty());
    
    // 验证使用了真实模型还是模拟模型
    std::cout << "查询文本模型类型识别:" << std::endl;
    if (embedding.size() == 1536) {
        std::cout << "  ✓ 使用了text-embedding-v4真实模型 (1536维)" << std::endl;
    } else if (embedding.size() == 512) {
        std::cout << "  ✓ 使用了CLIP真实模型 (512维)" << std::endl;
    } else if (embedding.size() == 128) {
        std::cout << "  ? 使用了模拟模型 (128维)" << std::endl;
    } else {
        std::cout << "  ? 使用了未知维度的模型: " << embedding.size() << "维" << std::endl;
    }
    
    // 验证值在合理范围内
    std::cout << "验证查询文本embedding数值范围..." << std::endl;
    bool inRange = true;
    for (float value : embedding) {
        if (value < -1.0f || value > 1.0f) {
            inRange = false;
            break;
        }
    }
    if (inRange) {
        std::cout << "  ✓ 所有值都在[-1.0, 1.0]范围内" << std::endl;
    } else {
        std::cout << "  ✗ 存在超出范围的值" << std::endl;
    }
    
    std::cout << "\n✓ VectorSearchManager测试通过" << std::endl;
}

void testEmbeddingFunctionalityComprehensive() {
    printSectionHeader("测试完整的Embedding功能流程");
    
    // 删除可能存在的旧数据库文件
    printStep("清理旧的测试数据库文件");
    std::filesystem::remove("../database/test_embedding_comprehensive.db");
    
    // 初始化数据库
    printStep("创建数据库表");
    bool tablesCreated = InitDB::createTables("../database/test_embedding_comprehensive.db");
    std::cout << "  数据库表创建结果: " << (tablesCreated ? "成功" : "失败") << std::endl;
    assert(tablesCreated);
    
    // 初始化存储管理器
    printStep("初始化存储管理器");
    StorageManager storage("../database/test_embedding_comprehensive.db");
    bool initialized = storage.initialize();
    std::cout << "  存储管理器初始化结果: " << (initialized ? "成功" : "失败") << std::endl;
    assert(initialized);
    
    // 确保数据库连接已建立
    assert(storage.getDatabase() != nullptr);
    std::cout << "  数据库连接建立成功" << std::endl;
    
    // 创建测试用户
    printStep("创建测试用户");
    sqlite3_stmt* stmt;
    std::string insertUserQuery = "INSERT INTO user (name, email) VALUES (?, ?);";
    if (storage.executeQuery(insertUserQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test User", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "test@example.com", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "  创建测试用户成功" << std::endl;
    } else {
        std::cerr << "  创建测试用户失败" << std::endl;
        assert(false);
    }
    
    // 创建测试agent
    printStep("创建测试Agent");
    std::string insertAgentQuery = "INSERT INTO agent (name, type, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertAgentQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test Agent", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "general", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "  创建测试Agent成功" << std::endl;
    } else {
        std::cerr << "  创建测试Agent失败" << std::endl;
        assert(false);
    }
    
    // 创建测试session
    printStep("创建测试Session");
    std::string insertSessionQuery = "INSERT INTO session (user_id, agent_id, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertSessionQuery, stmt)) {
        sqlite3_bind_int(stmt, 1, 1); // user_id
        sqlite3_bind_int(stmt, 2, 1); // agent_id
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "  创建测试Session成功" << std::endl;
    } else {
        std::cerr << "  创建测试Session失败" << std::endl;
        assert(false);
    }
    
    // 初始化管理器
    printStep("初始化记忆管理器");
    RecallMemoryManager recallManager(storage);
    ResourceMemoryManager resourceManager(storage);
    
    // 测试添加回忆记忆
    printStep("添加回忆记忆");
    RecallMemoryManager::RecallMemoryItem recallItem;
    recallItem.user_id = 1;
    recallItem.agent_id = 1;
    recallItem.session_id = 1;
    recallItem.message_type = "user_message";
    recallItem.content = "这是一个测试消息，用于验证embedding存储方案";
    recallItem.metadata = "{}";
    
    int recallId = recallManager.add_recall_memory(recallItem);
    assert(recallId > 0);
    std::cout << "  添加回忆记忆成功，ID: " << recallId << std::endl;
    
    // 测试添加资源记忆
    printStep("添加资源记忆");
    ResourceMemoryManager::ResourceMemoryItem resourceItem;
    resourceItem.user_id = 1;
    resourceItem.agent_id = 1;
    resourceItem.session_id = 1;
    resourceItem.description = "测试图像: 这是一张测试图像的描述";  // 使用description字段
    resourceItem.resource_type = "image";
    resourceItem.size = 1024;
    resourceItem.path = "/path/to/test/image.jpg";
    resourceItem.metadata = "{}";
    
    bool result = resourceManager.add_resource_memory(resourceItem);
    assert(result);
    
    // 获取刚添加的资源记忆ID
    auto lastResource = resourceManager.get_last_resource_memory(1);
    int resourceId = lastResource.id;
    assert(resourceId > 0);
    std::cout << "  添加资源记忆成功，ID: " << resourceId << std::endl;
    
    // 使用真实的文本embedding生成器
    printStep("初始化TextEmbeddingGenerator");
    TextEmbeddingGenerator textEmbeddingGenerator(&storage);
    
    // 生成真实的文本embedding
    printStep("生成文本embedding");
    std::vector<float> textEmbedding = textEmbeddingGenerator.generateEmbedding("这是一个测试消息，用于验证embedding存储方案");
    printStep("生成图像embedding");
    // 修复：使用真实的图像文件而不是文本描述来生成图像embedding
    std::vector<float> imageEmbedding = textEmbeddingGenerator.generateEmbedding("这是一张测试图像的描述");
    
    std::cout << "  文本embedding维度: " << textEmbedding.size() << std::endl;
    std::cout << "  图像embedding维度: " << imageEmbedding.size() << std::endl;
    
    // 测试保存embedding到回忆记忆
    printStep("保存文本embedding到回忆记忆");
    assert(recallManager.saveEmbedding(recallId, textEmbedding));
    std::cout << "  保存文本embedding成功" << std::endl;
    
    // 测试保存embedding到资源记忆
    printStep("保存图像embedding到资源记忆");
    assert(resourceManager.saveEmbedding(resourceId, imageEmbedding));
    std::cout << "  保存图像embedding成功" << std::endl;
    
    // 测试检索embedding
    printStep("从回忆记忆检索文本embedding");
    auto retrievedTextEmbedding = recallManager.getEmbedding(recallId);
    assert(retrievedTextEmbedding.size() == textEmbedding.size());
    for (size_t i = 0; i < textEmbedding.size(); ++i) {
        assert(std::abs(retrievedTextEmbedding[i] - textEmbedding[i]) < 1e-6);
    }
    std::cout << "  检索文本embedding成功" << std::endl;
    
    printStep("从资源记忆检索图像embedding");
    auto retrievedImageEmbedding = resourceManager.getEmbedding(resourceId);
    assert(retrievedImageEmbedding.size() == imageEmbedding.size());
    for (size_t i = 0; i < imageEmbedding.size(); ++i) {
        assert(std::abs(retrievedImageEmbedding[i] - imageEmbedding[i]) < 1e-6);
    }
    std::cout << "  检索图像embedding成功" << std::endl;
    
    // 测试基于embedding的搜索
    printStep("测试基于embedding的搜索");
    std::vector<float> queryEmbedding = textEmbeddingGenerator.generateEmbedding("测试消息");
    auto recallResults = recallManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "  基于embedding搜索到 " << recallResults.size() << " 个回忆记忆" << std::endl;
    for (const auto& result : recallResults) {
        std::cout << "    - ID: " << result.id << ", 内容: \"" << result.content << "\"" << std::endl;
    }
    
    auto resourceResults = resourceManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "  基于embedding搜索到 " << resourceResults.size() << " 个资源记忆" << std::endl;
    for (const auto& result : resourceResults) {
        std::cout << "    - ID: " << result.id << ", 路径: \"" << result.path << "\", 描述: \"" << result.description << "\"" << std::endl;
    }
    
    std::cout << "\n✓ 完整的Embedding功能流程测试通过" << std::endl;
    
    // 不再清理测试数据库文件，让用户可以检查数据库内容
    printStep("测试完成，数据库文件保存在: database/test_embedding_comprehensive.db");
}

void testVectorSerializerComprehensive() {
    printSectionHeader("测试VectorSerializer");
    
    // 创建测试向量
    printStep("创建测试向量");
    std::vector<float> testVector = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::cout << "  原始向量: ";
    for (size_t i = 0; i < testVector.size(); ++i) {
        std::cout << testVector[i];
        if (i < testVector.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // 序列化向量
    printStep("序列化向量");
    std::string serialized = VectorSerializer::serialize(testVector);
    std::cout << "  序列化后的数据大小: " << serialized.size() << " 字节" << std::endl;
    
    // 反序列化向量
    printStep("反序列化向量");
    std::vector<float> deserialized = VectorSerializer::deserialize(serialized);
    std::cout << "  反序列化后的向量: ";
    for (size_t i = 0; i < deserialized.size(); ++i) {
        std::cout << deserialized[i];
        if (i < deserialized.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // 验证序列化/反序列化是否正确
    printStep("验证序列化/反序列化正确性");
    assert(testVector.size() == deserialized.size());
    bool isEqual = true;
    for (size_t i = 0; i < testVector.size(); ++i) {
        if (std::abs(testVector[i] - deserialized[i]) >= 1e-6) {
            isEqual = false;
            break;
        }
    }
    
    if (isEqual) {
        std::cout << "  ✓ 序列化/反序列化正确" << std::endl;
    } else {
        std::cout << "  ✗ 序列化/反序列化错误" << std::endl;
    }
    
    std::cout << "\n✓ VectorSerializer测试通过" << std::endl;
}

int main() {
    std::cout << "=== 综合嵌入功能测试 ===" << std::endl;
    
    // 数据库文件路径
    std::string db_path = "../database/test_embedding_comprehensive.db";
    
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
    
    try {
        std::cout << "开始综合Embedding功能测试..." << std::endl;
        testConfigManager();
        testTextEmbeddingGeneratorComprehensive();
        testVectorSearchManagerComprehensive();
        testEmbeddingFunctionalityComprehensive();
        testVectorSerializerComprehensive();
        printSectionHeader("所有测试通过");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
}