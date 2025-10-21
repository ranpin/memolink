#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <filesystem>
#include <iomanip>

// 打印向量的辅助函数
void printVector(const std::vector<float>& vec, const std::string& label) {
    std::cout << label << ": [";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << std::fixed << std::setprecision(2) << vec[i];
    }
    std::cout << "]" << std::endl;
}

// 计算两个向量之间的余弦相似度
float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    if (normA == 0.0f || normB == 0.0f) {
        return 0.0f;
    }
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

void testEmbeddingFunctionality() {
    std::cout << "=== 测试Embedding功能 ===" << std::endl;
    
    // 删除可能存在的旧数据库文件
    std::filesystem::remove("test_embedding.db");
    
    // 初始化数据库
    std::cout << "Creating tables..." << std::endl;
    bool tablesCreated = InitDB::createTables("test_embedding.db");
    std::cout << "Tables created: " << (tablesCreated ? "true" : "false") << std::endl;
    assert(tablesCreated);
    
    std::cout << "Initializing storage manager..." << std::endl;
    StorageManager storage("test_embedding.db");
    bool initialized = storage.initialize();
    std::cout << "Storage initialized: " << (initialized ? "true" : "false") << std::endl;
    assert(initialized);
    
    // 确保数据库连接已建立
    if (!storage.getDatabase()) {
        std::cerr << "Failed to initialize database connection" << std::endl;
        assert(false);
    }
    
    std::cout << "Database connection established" << std::endl;
    
    // 先创建用户和agent以满足外键约束
    sqlite3_stmt* stmt;
    std::string insertUserQuery = "INSERT INTO user (name, email) VALUES (?, ?);";
    if (storage.executeQuery(insertUserQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test User", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "test@example.com", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert user: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare user insert statement" << std::endl;
    }
    
    std::string insertAgentQuery = "INSERT INTO agent (name, type, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertAgentQuery, stmt)) {
        sqlite3_bind_text(stmt, 1, "Test Agent", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "general", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert agent: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare agent insert statement" << std::endl;
    }
    
    // 创建一个session
    std::string insertSessionQuery = "INSERT INTO session (user_id, agent_id, status) VALUES (?, ?, ?);";
    if (storage.executeQuery(insertSessionQuery, stmt)) {
        sqlite3_bind_int(stmt, 1, 1); // user_id
        sqlite3_bind_int(stmt, 2, 1); // agent_id
        sqlite3_bind_text(stmt, 3, "active", -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert session: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to prepare session insert statement" << std::endl;
    }
    
    // 初始化记忆管理器
    RecallMemoryManager recallManager(storage);
    ResourceMemoryManager resourceManager(storage);
    
    // 添加多个回忆记忆用于测试相似度检索
    std::cout << "\n--- 添加多个回忆记忆用于相似度测试 ---" << std::endl;
    
    // 定义测试数据
    std::vector<std::pair<std::string, std::vector<float>>> testData = {
        {"今天天气很好，阳光明媚", {0.8f, 0.1f, 0.2f, 0.1f, 0.9f}},
        {"我喜欢在公园里散步", {0.7f, 0.2f, 0.3f, 0.2f, 0.8f}},
        {"计算机编程是一项有趣的技能", {0.1f, 0.8f, 0.9f, 0.7f, 0.2f}},
        {"音乐可以放松心情", {0.2f, 0.3f, 0.1f, 0.9f, 0.4f}},
        {"阅读是获取知识的好方法", {0.3f, 0.5f, 0.6f, 0.4f, 0.1f}},
        {"健康饮食对身体有益", {0.4f, 0.1f, 0.2f, 0.3f, 0.7f}},
        {"学习新语言很有挑战性", {0.5f, 0.6f, 0.4f, 0.5f, 0.3f}},
        {"旅行可以开阔视野", {0.6f, 0.4f, 0.5f, 0.6f, 0.5f}},
        {"运动有助于保持健康", {0.3f, 0.2f, 0.1f, 0.8f, 0.6f}},
        {"艺术创作需要灵感", {0.1f, 0.4f, 0.3f, 0.2f, 0.5f}}
    };
    
    // 添加回忆记忆
    for (size_t i = 0; i < testData.size(); ++i) {
        RecallMemoryManager::RecallMemoryItem recallItem;
        recallItem.user_id = 1;
        recallItem.agent_id = 1;
        recallItem.session_id = 1;
        recallItem.message_type = "text";
        recallItem.content = testData[i].first;
        recallItem.metadata = "{}";
        recallItem.embedding = "";
        recallItem.created_at = "";
        recallItem.updated_at = "";
        
        bool recallAdded = recallManager.add_recall_memory(recallItem);
        std::cout << "添加回忆记忆" << (i+1) << " (" << testData[i].first << "): " << (recallAdded ? "成功" : "失败") << std::endl;
        assert(recallAdded);
    }
    
    // 添加多种模态的资源记忆
    std::cout << "\n--- 添加多种模态的资源记忆 ---" << std::endl;
    
    // 定义资源记忆测试数据
    std::vector<std::tuple<std::string, std::string, std::string, std::vector<float>>> resourceTestData = {
        {"文档资源1", "text", "这是一份关于人工智能技术的文档", {0.1f, 0.2f, 0.3f, 0.4f, 0.5f}},
        {"图片资源1", "image", "这是一张美丽的风景图片", {0.2f, 0.3f, 0.4f, 0.5f, 0.6f}},
        {"音频资源1", "audio", "这是一段古典音乐录音", {0.3f, 0.4f, 0.5f, 0.6f, 0.7f}},
        {"视频资源1", "video", "这是一部关于自然的纪录片", {0.4f, 0.5f, 0.6f, 0.7f, 0.8f}},
        {"代码资源1", "code", "这是一个Python机器学习示例代码", {0.5f, 0.6f, 0.7f, 0.8f, 0.9f}},
        {"数据资源1", "data", "这是一份销售数据统计表", {0.6f, 0.7f, 0.8f, 0.9f, 0.1f}},
        {"模型资源1", "model", "这是一个预训练的图像识别模型", {0.7f, 0.8f, 0.9f, 0.1f, 0.2f}},
        {"网页资源1", "web", "这是一个技术博客网页", {0.8f, 0.9f, 0.1f, 0.2f, 0.3f}},
        {"书籍资源1", "book", "这是一本关于深度学习的书籍", {0.9f, 0.1f, 0.2f, 0.3f, 0.4f}},
        {"论文资源1", "paper", "这是一篇计算机视觉研究论文", {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}}
    };
    
    // 添加资源记忆
    for (size_t i = 0; i < resourceTestData.size(); ++i) {
        ResourceMemoryManager::ResourceMemoryItem resourceItem;
        resourceItem.user_id = 1;
        resourceItem.agent_id = 1;
        resourceItem.title = std::get<0>(resourceTestData[i]);
        resourceItem.resource_type = std::get<1>(resourceTestData[i]);
        resourceItem.content = std::get<2>(resourceTestData[i]);
        resourceItem.path = "/test/path/" + std::to_string(i+1);
        resourceItem.metadata = "{}";
        resourceItem.embedding = "";
        resourceItem.created_at = "";
        
        bool resourceAdded = resourceManager.add_resource_memory(resourceItem);
        std::cout << "添加资源记忆" << (i+1) << " (" << resourceItem.title << " - " << resourceItem.resource_type << "): " << (resourceAdded ? "成功" : "失败") << std::endl;
        assert(resourceAdded);
    }
    
    // 为回忆记忆保存不同的embedding向量
    std::cout << "\n--- 保存不同embedding向量 ---" << std::endl;
    
    for (size_t i = 0; i < testData.size(); ++i) {
        bool embeddingSaved = recallManager.saveEmbedding(i+1, testData[i].second);
        std::cout << "保存回忆记忆" << (i+1) << "的embedding: " << (embeddingSaved ? "成功" : "失败") << std::endl;
        assert(embeddingSaved);
    }
    
    // 为资源记忆保存embedding向量
    for (size_t i = 0; i < resourceTestData.size(); ++i) {
        bool embeddingSaved = resourceManager.saveEmbedding(i+1, std::get<3>(resourceTestData[i]));
        std::cout << "保存资源记忆" << (i+1) << "的embedding: " << (embeddingSaved ? "成功" : "失败") << std::endl;
        assert(embeddingSaved);
    }
    
    // 测试获取embedding
    std::cout << "\n--- 测试获取embedding ---" << std::endl;
    for (size_t i = 0; i < testData.size(); ++i) {
        auto retrievedEmbedding = recallManager.getEmbedding(i+1);
        assert(retrievedEmbedding.size() == testData[i].second.size());
        std::cout << "✓ 获取回忆记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    for (size_t i = 0; i < resourceTestData.size(); ++i) {
        auto retrievedEmbedding = resourceManager.getEmbedding(i+1);
        assert(retrievedEmbedding.size() == std::get<3>(resourceTestData[i]).size());
        std::cout << "✓ 获取资源记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    // 测试基于embedding的搜索
    std::cout << "\n--- 测试基于embedding的相似度搜索 ---" << std::endl;
    
    // 定义查询向量
    std::vector<std::pair<std::string, std::vector<float>>> queryData = {
        {"天气相关查询", {0.75f, 0.15f, 0.25f, 0.15f, 0.85f}},
        {"技术相关查询", {0.15f, 0.75f, 0.85f, 0.65f, 0.25f}},
        {"音乐相关查询", {0.25f, 0.35f, 0.15f, 0.85f, 0.45f}},
        {"学习相关查询", {0.45f, 0.55f, 0.55f, 0.45f, 0.25f}},
        {"健康相关查询", {0.35f, 0.15f, 0.15f, 0.75f, 0.65f}}
    };
    
    // 对每个查询向量进行测试
    for (const auto& query : queryData) {
        std::cout << "\n使用" << query.first << "向量搜索:" << std::endl;
        printVector(query.second, "查询向量");
        
        auto recallSearchResults = recallManager.searchByEmbedding(1, query.second, 5);
        std::cout << "找到 " << recallSearchResults.size() << " 个回忆记忆结果:" << std::endl;
        
        // 显示每个结果及其相似度
        for (const auto& result : recallSearchResults) {
            // 获取该记忆的embedding
            auto memoryEmbedding = recallManager.getEmbedding(result.id);
            // 计算相似度
            float similarity = cosineSimilarity(query.second, memoryEmbedding);
            
            std::cout << "  ID: " << result.id 
                      << ", 内容: " << result.content 
                      << ", 相似度: " << std::fixed << std::setprecision(4) << similarity << std::endl;
            printVector(memoryEmbedding, "    记忆向量");
        }
        
        // 应该至少找到一些结果
        assert(recallSearchResults.size() > 0);
        std::cout << "✓ " << query.first << "向量搜索成功" << std::endl;
    }
    
    // 测试资源记忆搜索
    std::cout << "\n--- 测试资源记忆向量搜索 ---" << std::endl;
    
    // 定义资源查询向量
    std::vector<std::pair<std::string, std::vector<float>>> resourceQueryData = {
        {"文档相关查询", {0.15f, 0.25f, 0.35f, 0.45f, 0.55f}},
        {"图像相关查询", {0.25f, 0.35f, 0.45f, 0.55f, 0.65f}},
        {"音频相关查询", {0.35f, 0.45f, 0.55f, 0.65f, 0.75f}},
        {"视频相关查询", {0.45f, 0.55f, 0.65f, 0.75f, 0.85f}},
        {"代码相关查询", {0.55f, 0.65f, 0.75f, 0.85f, 0.95f}}
    };
    
    // 对每个资源查询向量进行测试
    for (const auto& query : resourceQueryData) {
        std::cout << "\n使用" << query.first << "向量搜索:" << std::endl;
        printVector(query.second, "资源查询向量");
        
        auto resourceSearchResults = resourceManager.searchByEmbedding(1, query.second, 5);
        std::cout << "找到 " << resourceSearchResults.size() << " 个资源记忆结果:" << std::endl;
        
        // 显示每个结果及其相似度
        for (const auto& result : resourceSearchResults) {
            // 获取该记忆的embedding
            auto memoryEmbedding = resourceManager.getEmbedding(result.id);
            // 计算相似度
            float similarity = cosineSimilarity(query.second, memoryEmbedding);
            
            std::cout << "  ID: " << result.id 
                      << ", 标题: " << result.title
                      << ", 类型: " << result.resource_type
                      << ", 内容: " << result.content 
                      << ", 相似度: " << std::fixed << std::setprecision(4) << similarity << std::endl;
            printVector(memoryEmbedding, "    资源向量");
        }
        
        // 应该至少找到一些结果
        assert(resourceSearchResults.size() > 0);
        std::cout << "✓ " << query.first << "资源向量搜索成功" << std::endl;
    }
    
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
}

int main() {
    testEmbeddingFunctionality();
    return 0;
}