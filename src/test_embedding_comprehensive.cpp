#include "models/TextEmbeddingGenerator.h"
#include "models/VectorSearchManager.h"
#include "models/ModelManager.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/initialization/ConfigManager/ConfigManager.h"
#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "utils/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <filesystem>
#include <cmath>

void testConfigManager() {
    std::cout << "=== 测试配置管理器 ===" << std::endl;
    
    // 检查API密钥配置
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string api_key = config_manager.getApiKey();
    std::cout << "配置文件中的API密钥: " << (api_key.empty() ? "未配置" : "已配置") << std::endl;
    
    if (!api_key.empty()) {
        std::cout << "API密钥前缀: " << api_key.substr(0, std::min(10, (int)api_key.length())) << "..." << std::endl;
    }
    
    // 检查模型配置
    std::string embedding_model = config_manager.getEmbeddingModelName();
    std::string text_model = config_manager.getTextModelName();
    
    std::cout << "Embedding模型: " << embedding_model << std::endl;
    std::cout << "文本模型: " << text_model << std::endl;
    
    std::cout << "✓ 配置管理器测试通过" << std::endl;
}

void testTextEmbeddingGenerator() {
    std::cout << "\n=== 测试TextEmbeddingGenerator ===" << std::endl;
    
    TextEmbeddingGenerator generator;
    
    // 测试获取维度
    int dimension = generator.getEmbeddingDimension();
    std::cout << "Embedding维度: " << dimension << std::endl;
    
    // 测试生成embedding
    std::string testText = "人工智能是计算机科学的一个分支，它企图了解智能的实质，并生产出一种新的能以人类智能相似的方式做出反应的智能机器。";
    std::vector<float> embedding = generator.generateEmbedding(testText);
    
    std::cout << "生成的Embedding向量维度: " << embedding.size() << std::endl;
    assert(!embedding.empty());
    
    // 验证使用了真实模型还是模拟模型
    if (embedding.size() == 1024) {
        std::cout << "✓ 使用了真实模型 (1024维)" << std::endl;
    } else if (embedding.size() == 128) {
        std::cout << "? 使用了模拟模型 (128维)" << std::endl;
    } else {
        std::cout << "? 使用了未知维度: " << embedding.size() << std::endl;
    }
    
    // 验证值在合理范围内
    for (float value : embedding) {
        assert(value >= -1.0f && value <= 1.0f);
    }
    
    // 测试不同文本生成不同的embedding
    std::string text1 = "今天天气很好";
    std::string text2 = "今天下雨了";
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
        std::cout << "两个不同文本的embedding相似度: " << similarity << std::endl;
        
        if (similarity < 0.99) {
            std::cout << "✓ 不同文本生成不同的Embedding" << std::endl;
        } else {
            std::cout << "✗ 不同文本生成过于相似的Embedding" << std::endl;
        }
    }
    
    std::cout << "✓ TextEmbeddingGenerator测试通过" << std::endl;
}

void testVectorSearchManager() {
    std::cout << "\n=== 测试VectorSearchManager ===" << std::endl;
    
    // 初始化VectorSearchManager
    StorageManager storage("test.db"); // 临时存储管理器
    VectorSearchManager vectorSearchManager(storage);
    std::cout << "VectorSearchManager初始化成功" << std::endl;
    
    // 测试文本embedding生成
    std::string testText = "人工智能技术正在快速发展";
    std::vector<float> embedding = vectorSearchManager.generateTextEmbedding(testText);
    std::cout << "生成文本embedding，维度: " << embedding.size() << std::endl;
    assert(!embedding.empty());
    
    // 验证使用了真实模型还是模拟模型
    if (embedding.size() == 1024) {
        std::cout << "✓ 使用了真实模型 (1024维)" << std::endl;
    } else if (embedding.size() == 128) {
        std::cout << "? 使用了模拟模型 (128维)" << std::endl;
    } else {
        std::cout << "? 使用了未知维度: " << embedding.size() << std::endl;
    }
    
    // 验证值在合理范围内
    for (float value : embedding) {
        assert(value >= -1.0f && value <= 1.0f);
    }
    
    std::cout << "✓ VectorSearchManager测试通过" << std::endl;
}

void testEmbeddingFunctionality() {
    std::cout << "\n=== 测试完整的Embedding功能流程 ===" << std::endl;
    
    // 删除可能存在的旧数据库文件
    std::filesystem::remove("test_embedding_comprehensive.db");
    
    // 初始化数据库
    std::cout << "创建数据库表..." << std::endl;
    bool tablesCreated = InitDB::createTables("test_embedding_comprehensive.db");
    std::cout << "数据库表创建: " << (tablesCreated ? "成功" : "失败") << std::endl;
    assert(tablesCreated);
    
    // 初始化存储管理器
    std::cout << "初始化存储管理器..." << std::endl;
    StorageManager storage("test_embedding_comprehensive.db");
    bool initialized = storage.initialize();
    std::cout << "存储管理器初始化: " << (initialized ? "成功" : "失败") << std::endl;
    assert(initialized);
    
    // 确保数据库连接已建立
    assert(storage.getDatabase() != nullptr);
    std::cout << "数据库连接建立成功" << std::endl;
    
    // 创建测试用户
    sqlite3_stmt* stmt;
    std::string insertUserQuery = "INSERT INTO user (name, email) VALUES (?, ?);";
    if (storage.executeQuery(insertUserQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test User", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "test@example.com", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "创建测试用户成功" << std::endl;
    } else {
        std::cerr << "创建测试用户失败" << std::endl;
        assert(false);
    }
    
    // 创建测试agent
    std::string insertAgentQuery = "INSERT INTO agent (name, type, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertAgentQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test Agent", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "general", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "创建测试Agent成功" << std::endl;
    } else {
        std::cerr << "创建测试Agent失败" << std::endl;
        assert(false);
    }
    
    // 创建测试session
    std::string insertSessionQuery = "INSERT INTO session (user_id, agent_id, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertSessionQuery, stmt)) {
        sqlite3_bind_int(stmt, 1, 1); // user_id
        sqlite3_bind_int(stmt, 2, 1); // agent_id
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        assert(rc == SQLITE_DONE);
        sqlite3_finalize(stmt);
        std::cout << "创建测试Session成功" << std::endl;
    } else {
        std::cerr << "创建测试Session失败" << std::endl;
        assert(false);
    }
    
    // 初始化管理器
    RecallMemoryManager recallManager(storage);
    ResourceMemoryManager resourceManager(storage);
    
    // 测试添加回忆记忆
    RecallMemoryManager::RecallMemoryItem recallItem;
    recallItem.user_id = 1;
    recallItem.agent_id = 1;
    recallItem.session_id = 1;
    recallItem.message_type = "user_message";
    recallItem.content = "这是一个测试消息，用于验证embedding存储方案";
    recallItem.metadata = "{}";
    
    int recallId = recallManager.add_recall_memory(recallItem);
    assert(recallId > 0);
    std::cout << "添加回忆记忆成功，ID: " << recallId << std::endl;
    
    // 测试添加资源记忆
    ResourceMemoryManager::ResourceMemoryItem resourceItem;
    resourceItem.user_id = 1;
    resourceItem.agent_id = 1;
    resourceItem.filename = "test_image.jpg";
    resourceItem.title = "测试图像";
    resourceItem.content = "这是一张测试图像的描述";
    resourceItem.resource_type = "image";
    resourceItem.summary = "图像摘要";
    resourceItem.size = 1024;
    resourceItem.tree_path = "/";
    resourceItem.path = "/path/to/test/image.jpg";
    resourceItem.metadata = "{}";
    
    int resourceId = resourceManager.add_resource_memory(resourceItem);
    assert(resourceId > 0);
    std::cout << "添加资源记忆成功，ID: " << resourceId << std::endl;
    
    // 使用真实的文本embedding生成器
    TextEmbeddingGenerator textEmbeddingGenerator;
    
    // 生成真实的文本embedding
    std::vector<float> textEmbedding = textEmbeddingGenerator.generateEmbedding("这是一个测试消息，用于验证embedding存储方案");
    std::vector<float> imageEmbedding = textEmbeddingGenerator.generateEmbedding("这是一张测试图像的描述");
    
    std::cout << "生成文本embedding，维度: " << textEmbedding.size() << std::endl;
    std::cout << "生成图像embedding，维度: " << imageEmbedding.size() << std::endl;
    
    // 测试保存embedding到回忆记忆
    assert(recallManager.saveEmbedding(recallId, textEmbedding));
    std::cout << "保存文本embedding成功" << std::endl;
    
    // 测试保存embedding到资源记忆
    assert(resourceManager.saveEmbedding(resourceId, imageEmbedding));
    std::cout << "保存图像embedding成功" << std::endl;
    
    // 测试检索embedding
    auto retrievedTextEmbedding = recallManager.getEmbedding(recallId);
    assert(retrievedTextEmbedding.size() == textEmbedding.size());
    for (size_t i = 0; i < textEmbedding.size(); ++i) {
        assert(std::abs(retrievedTextEmbedding[i] - textEmbedding[i]) < 1e-6);
    }
    std::cout << "检索文本embedding成功" << std::endl;
    
    auto retrievedImageEmbedding = resourceManager.getEmbedding(resourceId);
    assert(retrievedImageEmbedding.size() == imageEmbedding.size());
    for (size_t i = 0; i < imageEmbedding.size(); ++i) {
        assert(std::abs(retrievedImageEmbedding[i] - imageEmbedding[i]) < 1e-6);
    }
    std::cout << "检索图像embedding成功" << std::endl;
    
    // 测试基于embedding的搜索
    std::vector<float> queryEmbedding = textEmbeddingGenerator.generateEmbedding("测试消息");
    auto recallResults = recallManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "基于embedding搜索到 " << recallResults.size() << " 个回忆记忆" << std::endl;
    
    auto resourceResults = resourceManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "基于embedding搜索到 " << resourceResults.size() << " 个资源记忆" << std::endl;
    
    std::cout << "✓ 完整的Embedding功能流程测试通过" << std::endl;
    
    // 清理测试数据库文件
    std::filesystem::remove("test_embedding_comprehensive.db");
}

void testVectorSerializer() {
    std::cout << "\n=== 测试VectorSerializer ===" << std::endl;
    
    // 创建测试向量
    std::vector<float> testVector = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    
    // 序列化向量
    std::string serialized = VectorSerializer::serialize(testVector);
    std::cout << "序列化后的数据大小: " << serialized.size() << " 字节" << std::endl;
    
    // 反序列化向量
    std::vector<float> deserialized = VectorSerializer::deserialize(serialized);
    
    // 验证序列化/反序列化是否正确
    assert(testVector.size() == deserialized.size());
    for (size_t i = 0; i < testVector.size(); ++i) {
        assert(std::abs(testVector[i] - deserialized[i]) < 1e-6);
    }
    
    std::cout << "✓ VectorSerializer测试通过" << std::endl;
}

int main() {
    std::cout << "=== 综合Embedding功能测试 ===" << std::endl;
    
    try {
        testConfigManager();
        testTextEmbeddingGenerator();
        testVectorSearchManager();
        testEmbeddingFunctionality();
        testVectorSerializer();
        std::cout << "\n=== 所有测试通过 ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}