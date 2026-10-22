#include "legacy/core-v1/RecallMemoryManager.h"
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "infra/storage/StorageManager.h"
#include "infra/storage/InitDB.h"
#include "infra/embedding/TextEmbeddingGenerator.h"
#include "legacy/infra-alt/ModelManager.h"
#include "legacy/desktop/capture/MultimodalDataManager.h"
#include "infra/storage/VectorSerializer.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <filesystem>
#include <cmath>

void printSeparatorEmbedding() {
    std::cout << "\n" << std::string(50, '=') << "\n" << std::endl;
}

void printSubSeparatorEmbedding() {
    std::cout << "\n" << std::string(30, '-') << "\n" << std::endl;
}

void testEmbeddingFunctionalityMain() {
    printSeparatorEmbedding();
    std::cout << "=== 测试Embedding功能 ===" << std::endl;
    printSeparatorEmbedding();
    
    // 删除可能存在的旧数据库文件
    std::cout << "清理旧的测试数据库文件..." << std::endl;
    std::filesystem::remove("test_embedding.db");
    
    // 初始化数据库
    std::cout << "创建数据库表..." << std::endl;
    bool tablesCreated = InitDB::createTables("test_embedding.db");
    std::cout << "数据库表创建结果: " << (tablesCreated ? "成功" : "失败") << std::endl;
    assert(tablesCreated);
    
    // 初始化存储管理器
    std::cout << "初始化存储管理器..." << std::endl;
    StorageManager storage("test_embedding.db");
    bool initialized = storage.initialize();
    std::cout << "存储管理器初始化结果: " << (initialized ? "成功" : "失败") << std::endl;
    assert(initialized);
    
    // 确保数据库连接已建立
    assert(storage.getDatabase() != nullptr);
    std::cout << "数据库连接建立成功" << std::endl;
    
    // 创建测试用户
    std::cout << "创建测试用户..." << std::endl;
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
    std::cout << "创建测试Agent..." << std::endl;
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
    std::cout << "创建测试Session..." << std::endl;
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
    std::cout << "初始化记忆管理器..." << std::endl;
    RecallMemoryManager recallManager(storage);
    ResourceMemoryManager resourceManager(storage);
    MultimodalDataManager multimodalDataManager("test_embedding.db");
    
    // 测试添加回忆记忆
    std::cout << "添加回忆记忆..." << std::endl;
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
    std::cout << "添加资源记忆..." << std::endl;
    ResourceMemoryManager::ResourceMemoryItem resourceItem;
    resourceItem.user_id = 1;
    resourceItem.agent_id = 1;
    resourceItem.session_id = 1;  // 使用创建的session ID
    resourceItem.path = "/path/to/test/image.jpg";
    resourceItem.description = "测试图像: 这是一张测试图像的描述";  // 使用description字段
    resourceItem.resource_type = "image";
    resourceItem.size = 1024;
    resourceItem.metadata = "{}";
    resourceItem.embedding = "";  // 初始化为空字符串
    
    bool result = resourceManager.add_resource_memory(resourceItem);
    assert(result);
    std::cout << "添加资源记忆成功" << std::endl;
    
    // 获取刚添加的资源记忆ID
    auto lastResource = resourceManager.get_last_resource_memory(1);
    int resourceId = lastResource.id;
    assert(resourceId > 0);
    
    // 使用CLIP处理器生成embedding
    std::cout << "初始化CLIP处理器..." << std::endl;
    TextEmbeddingGenerator clipProcessor(&storage);
    
    // 生成文本和图像embedding
    std::cout << "生成文本embedding..." << std::endl;
    std::vector<float> textEmbedding = clipProcessor.generateEmbedding("这是一个测试消息，用于验证embedding存储方案");
    std::cout << "生成图像embedding..." << std::endl;
    // 修复：使用正确的图像文件路径
    std::vector<float> imageEmbedding = multimodalDataManager.generateImageEmbedding("test_images/sample_image.png");
    
    std::cout << "文本embedding维度: " << textEmbedding.size() << std::endl;
    std::cout << "图像embedding维度: " << imageEmbedding.size() << std::endl;
    
    // 测试保存embedding
    std::cout << "保存文本embedding到回忆记忆..." << std::endl;
    assert(recallManager.saveEmbedding(recallId, textEmbedding));
    std::cout << "保存文本embedding成功" << std::endl;
    
    std::cout << "保存图像embedding到资源记忆..." << std::endl;
    assert(resourceManager.saveEmbedding(resourceId, imageEmbedding));
    std::cout << "保存图像embedding成功" << std::endl;
    
    // 测试检索embedding
    std::cout << "从回忆记忆检索文本embedding..." << std::endl;
    auto retrievedTextEmbedding = recallManager.getEmbedding(recallId);
    assert(retrievedTextEmbedding.size() == textEmbedding.size());
    // 验证embedding值的准确性
    for (size_t i = 0; i < textEmbedding.size(); ++i) {
        assert(std::abs(retrievedTextEmbedding[i] - textEmbedding[i]) < 1e-6);
    }
    std::cout << "检索文本embedding成功" << std::endl;
    
    std::cout << "从资源记忆检索图像embedding..." << std::endl;
    auto retrievedImageEmbedding = resourceManager.getEmbedding(resourceId);
    assert(retrievedImageEmbedding.size() == imageEmbedding.size());
    // 验证embedding值的准确性
    for (size_t i = 0; i < imageEmbedding.size(); ++i) {
        assert(std::abs(retrievedImageEmbedding[i] - imageEmbedding[i]) < 1e-6);
    }
    std::cout << "检索图像embedding成功" << std::endl;
    
    // 测试基于embedding的搜索
    std::cout << "测试基于embedding的搜索..." << std::endl;
    std::vector<float> queryEmbedding = clipProcessor.generateEmbedding("测试消息");
    auto recallResults = recallManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "基于embedding搜索到 " << recallResults.size() << " 个回忆记忆" << std::endl;
    
    auto resourceResults = resourceManager.searchByEmbedding(1, queryEmbedding, 5);
    std::cout << "基于embedding搜索到 " << resourceResults.size() << " 个资源记忆" << std::endl;
    
    std::cout << "\n✓ Embedding功能测试通过" << std::endl;
    
    // 清理测试数据库文件
    std::cout << "清理测试数据库文件..." << std::endl;
    std::filesystem::remove("test_embedding.db");
}

int main() {
    std::cout << "=== 嵌入和向量搜索功能测试 ===" << std::endl;
    
    // 数据库文件路径
    std::string db_path = "../database/test_embedding.db";
    
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
        testEmbeddingFunctionalityMain();
        std::cout << "\n=== 所有测试通过 ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
}