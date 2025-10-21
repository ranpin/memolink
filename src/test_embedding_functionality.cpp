#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "models/TextEmbeddingGenerator.h"
#include "utils/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <cmath>

void testEmbeddingFunctionality() {
    std::cout << "=== 测试Embedding功能 ===" << std::endl;
    
    // 删除可能存在的旧数据库文件
    std::filesystem::remove("test_embedding.db");
    
    // 初始化数据库
    std::cout << "创建数据库表..." << std::endl;
    bool tablesCreated = InitDB::createTables("test_embedding.db");
    std::cout << "数据库表创建: " << (tablesCreated ? "成功" : "失败") << std::endl;
    assert(tablesCreated);
    
    // 初始化存储管理器
    std::cout << "初始化存储管理器..." << std::endl;
    StorageManager storage("test_embedding.db");
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
    recallItem.session_id = 1;  // 使用创建的session ID
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
    
    std::cout << "=== Embedding功能测试通过 ===" << std::endl;
    
    // 清理测试数据库文件
    std::filesystem::remove("test_embedding.db");
}

int main() {
    std::cout << "=== Embedding功能测试 ===" << std::endl;
    
    try {
        testEmbeddingFunctionality();
        std::cout << "\n=== 所有测试通过 ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}