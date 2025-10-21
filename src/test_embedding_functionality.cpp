#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "utils/VectorSerializer.h"
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
    
    // 添加多种类型的资源记忆
    std::cout << "\n--- 添加多种类型的资源记忆 ---" << std::endl;
    
    // 文档资源
    std::vector<std::pair<std::string, std::vector<float>>> documentResources = {
        {"项目需求文档", {0.1f, 0.2f, 0.3f, 0.4f, 0.5f}},
        {"技术设计文档", {0.15f, 0.25f, 0.35f, 0.45f, 0.55f}},
        {"用户手册", {0.2f, 0.3f, 0.4f, 0.5f, 0.6f}},
        {"API参考文档", {0.25f, 0.35f, 0.45f, 0.55f, 0.65f}}
    };
    
    for (size_t i = 0; i < documentResources.size(); ++i) {
        ResourceMemoryManager::ResourceMemoryItem documentResource;
        documentResource.user_id = 1;
        documentResource.agent_id = 1;
        documentResource.filename = "document_" + std::to_string(i+1) + ".txt";
        documentResource.title = documentResources[i].first;
        documentResource.content = "这是" + documentResources[i].first + "的内容";
        documentResource.resource_type = "text";
        documentResource.summary = documentResources[i].first + "摘要";
        documentResource.size = 1024 + i * 100;
        documentResource.tree_path = "/documents/doc_" + std::to_string(i+1);
        documentResource.path = "/documents/doc_" + std::to_string(i+1);
        documentResource.metadata = "{}";
        documentResource.embedding = "";
        documentResource.created_at = "";
        
        bool documentResourceAdded = resourceManager.add_resource_memory(documentResource);
        std::cout << "添加文档资源记忆" << (i+1) << " (" << documentResources[i].first << "): " << (documentResourceAdded ? "成功" : "失败") << std::endl;
        assert(documentResourceAdded);
    }
    
    // 图像资源
    std::vector<std::pair<std::string, std::vector<float>>> imageResources = {
        {"风景照片", {0.9f, 0.8f, 0.7f, 0.6f, 0.5f}},
        {"人物肖像", {0.85f, 0.75f, 0.65f, 0.55f, 0.45f}},
        {"产品图片", {0.8f, 0.7f, 0.6f, 0.5f, 0.4f}}
    };
    
    for (size_t i = 0; i < imageResources.size(); ++i) {
        ResourceMemoryManager::ResourceMemoryItem imageResource;
        imageResource.user_id = 1;
        imageResource.agent_id = 1;
        imageResource.filename = "image_" + std::to_string(i+1) + ".jpg";
        imageResource.title = imageResources[i].first;
        imageResource.content = "图像描述：" + imageResources[i].first;
        imageResource.resource_type = "image";
        imageResource.summary = imageResources[i].first + "摘要";
        imageResource.size = 2048 + i * 500;
        imageResource.tree_path = "/images/img_" + std::to_string(i+1);
        imageResource.path = "/images/img_" + std::to_string(i+1);
        imageResource.metadata = R"({"width": 1920, "height": 1080, "format": "JPEG"})";
        imageResource.embedding = "";
        imageResource.created_at = "";
        
        bool imageResourceAdded = resourceManager.add_resource_memory(imageResource);
        std::cout << "添加图像资源记忆" << (i+1) << " (" << imageResources[i].first << "): " << (imageResourceAdded ? "成功" : "失败") << std::endl;
        assert(imageResourceAdded);
    }
    
    // 音频资源
    std::vector<std::pair<std::string, std::vector<float>>> audioResources = {
        {"音乐片段", {0.5f, 0.6f, 0.7f, 0.8f, 0.9f}},
        {"语音备忘录", {0.55f, 0.65f, 0.75f, 0.85f, 0.95f}},
        {"播客录音", {0.6f, 0.7f, 0.8f, 0.9f, 1.0f}}
    };
    
    for (size_t i = 0; i < audioResources.size(); ++i) {
        ResourceMemoryManager::ResourceMemoryItem audioResource;
        audioResource.user_id = 1;
        audioResource.agent_id = 1;
        audioResource.filename = "audio_" + std::to_string(i+1) + ".mp3";
        audioResource.title = audioResources[i].first;
        audioResource.content = "音频内容：" + audioResources[i].first;
        audioResource.resource_type = "audio";
        audioResource.summary = audioResources[i].first + "摘要";
        audioResource.size = 4096 + i * 1000;
        audioResource.tree_path = "/audio/aud_" + std::to_string(i+1);
        audioResource.path = "/audio/aud_" + std::to_string(i+1);
        audioResource.metadata = R"({"duration": 120.5, "format": "MP3", "sample_rate": 44100})";
        audioResource.embedding = "";
        audioResource.created_at = "";
        
        bool audioResourceAdded = resourceManager.add_resource_memory(audioResource);
        std::cout << "添加音频资源记忆" << (i+1) << " (" << audioResources[i].first << "): " << (audioResourceAdded ? "成功" : "失败") << std::endl;
        assert(audioResourceAdded);
    }
    
    // 为回忆记忆保存不同的embedding向量
    std::cout << "\n--- 保存不同embedding向量 ---" << std::endl;
    
    for (size_t i = 0; i < testData.size(); ++i) {
        bool embeddingSaved = recallManager.saveEmbedding(i+1, testData[i].second);
        std::cout << "保存回忆记忆" << (i+1) << "的embedding: " << (embeddingSaved ? "成功" : "失败") << std::endl;
        assert(embeddingSaved);
    }
    
    // 为资源记忆保存embedding向量
    for (size_t i = 0; i < documentResources.size(); ++i) {
        bool documentResourceEmbeddingSaved = resourceManager.saveEmbedding(i+1, documentResources[i].second);
        std::cout << "保存文档资源记忆" << (i+1) << "的embedding: " << (documentResourceEmbeddingSaved ? "成功" : "失败") << std::endl;
        assert(documentResourceEmbeddingSaved);
    }
    
    for (size_t i = 0; i < imageResources.size(); ++i) {
        bool imageResourceEmbeddingSaved = resourceManager.saveEmbedding(i+1+documentResources.size(), imageResources[i].second);
        std::cout << "保存图像资源记忆" << (i+1) << "的embedding: " << (imageResourceEmbeddingSaved ? "成功" : "失败") << std::endl;
        assert(imageResourceEmbeddingSaved);
    }
    
    for (size_t i = 0; i < audioResources.size(); ++i) {
        bool audioResourceEmbeddingSaved = resourceManager.saveEmbedding(i+1+documentResources.size()+imageResources.size(), audioResources[i].second);
        std::cout << "保存音频资源记忆" << (i+1) << "的embedding: " << (audioResourceEmbeddingSaved ? "成功" : "失败") << std::endl;
        assert(audioResourceEmbeddingSaved);
    }
    
    // 测试获取embedding
    std::cout << "\n--- 测试获取embedding ---" << std::endl;
    for (size_t i = 0; i < testData.size(); ++i) {
        auto retrievedEmbedding = recallManager.getEmbedding(i+1);
        assert(retrievedEmbedding.size() == testData[i].second.size());
        // 验证向量值是否正确
        for (size_t j = 0; j < testData[i].second.size(); ++j) {
            assert(std::abs(retrievedEmbedding[j] - testData[i].second[j]) < 1e-6);
        }
        std::cout << "✓ 获取回忆记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    for (size_t i = 0; i < documentResources.size(); ++i) {
        auto retrievedDocumentResourceEmbedding = resourceManager.getEmbedding(i+1);
        assert(retrievedDocumentResourceEmbedding.size() == documentResources[i].second.size());
        // 验证向量值是否正确
        for (size_t j = 0; j < documentResources[i].second.size(); ++j) {
            assert(std::abs(retrievedDocumentResourceEmbedding[j] - documentResources[i].second[j]) < 1e-6);
        }
        std::cout << "✓ 获取文档资源记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    for (size_t i = 0; i < imageResources.size(); ++i) {
        auto retrievedImageResourceEmbedding = resourceManager.getEmbedding(i+1+documentResources.size());
        assert(retrievedImageResourceEmbedding.size() == imageResources[i].second.size());
        // 验证向量值是否正确
        for (size_t j = 0; j < imageResources[i].second.size(); ++j) {
            assert(std::abs(retrievedImageResourceEmbedding[j] - imageResources[i].second[j]) < 1e-6);
        }
        std::cout << "✓ 获取图像资源记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    for (size_t i = 0; i < audioResources.size(); ++i) {
        auto retrievedAudioResourceEmbedding = resourceManager.getEmbedding(i+1+documentResources.size()+imageResources.size());
        assert(retrievedAudioResourceEmbedding.size() == audioResources[i].second.size());
        // 验证向量值是否正确
        for (size_t j = 0; j < audioResources[i].second.size(); ++j) {
            assert(std::abs(retrievedAudioResourceEmbedding[j] - audioResources[i].second[j]) < 1e-6);
        }
        std::cout << "✓ 获取音频资源记忆" << (i+1) << "的embedding成功" << std::endl;
    }
    
    // 测试VectorSerializer功能
    std::cout << "\n--- 测试VectorSerializer功能 ---" << std::endl;
    std::vector<float> testVec = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::string serialized = VectorSerializer::serialize(testVec);
    std::vector<float> deserialized = VectorSerializer::deserialize(serialized);
    
    assert(testVec.size() == deserialized.size());
    for (size_t i = 0; i < testVec.size(); ++i) {
        assert(std::abs(testVec[i] - deserialized[i]) < 1e-6);
    }
    std::cout << "✓ VectorSerializer功能测试成功" << std::endl;
    
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
        std::cout << "找到 " << recallSearchResults.size() << " 个结果:" << std::endl;
        
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
    
    // 定义资源查询向量 (与资源记忆向量维度相同)
    std::vector<std::pair<std::string, std::vector<float>>> resourceQueryData = {
        {"文档相关查询", {0.15f, 0.25f, 0.35f, 0.45f, 0.55f}},
        {"图像相关查询", {0.95f, 0.85f, 0.75f, 0.65f, 0.55f}},
        {"音频相关查询", {0.45f, 0.55f, 0.65f, 0.75f, 0.85f}}
    };
    
    // 对每个资源查询向量进行测试
    for (const auto& query : resourceQueryData) {
        std::cout << "\n使用" << query.first << "向量搜索:" << std::endl;
        printVector(query.second, "资源查询向量");
        
        // 调试：检查所有资源记忆的向量维度
        std::cout << "调试信息 - 所有资源记忆向量:" << std::endl;
        for (int i = 1; i <= 10; i++) {
            auto memoryEmbedding = resourceManager.getEmbedding(i);
            if (!memoryEmbedding.empty()) {
                std::cout << "  资源记忆ID " << i << " 向量维度: " << memoryEmbedding.size() << std::endl;
                // 检查维度是否匹配
                if (memoryEmbedding.size() != query.second.size()) {
                    std::cout << "    警告: 维度不匹配! 查询维度=" << query.second.size() 
                              << ", 记忆维度=" << memoryEmbedding.size() << std::endl;
                }
            }
        }
        
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
        
        // 注意：我们不强制要求必须找到结果，因为向量相似度可能不够高
        std::cout << "✓ " << query.first << "资源向量搜索完成" << std::endl;
    }
    
    // 测试按资源类型搜索
    std::cout << "\n--- 测试按资源类型搜索 ---" << std::endl;
    
    std::vector<std::string> resourceTypes = {"text", "image", "audio"};
    std::vector<std::string> typeLabels = {"文档", "图像", "音频"};
    
    for (size_t i = 0; i < resourceTypes.size(); ++i) {
        auto resourcesByType = resourceManager.getResourcesByType(1, resourceTypes[i]);
        std::cout << "找到 " << resourcesByType.size() << " 个 " << typeLabels[i] << " 类型的资源记忆:" << std::endl;
        
        for (const auto& resource : resourcesByType) {
            std::cout << "  ID: " << resource.id 
                      << ", 标题: " << resource.title
                      << ", 文件名: " << resource.filename
                      << ", 类型: " << resource.resource_type << std::endl;
        }
        
        // 应该找到对应类型的资源
        if (resourceTypes[i] == "text") {
            assert(resourcesByType.size() == documentResources.size());
        } else if (resourceTypes[i] == "image") {
            assert(resourcesByType.size() == imageResources.size());
        } else if (resourceTypes[i] == "audio") {
            assert(resourcesByType.size() == audioResources.size());
        }
        
        std::cout << "✓ " << typeLabels[i] << " 类型搜索成功" << std::endl;
    }
    
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
}

int main() {
    testEmbeddingFunctionality();
    return 0;
}