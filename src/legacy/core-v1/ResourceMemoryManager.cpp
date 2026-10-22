#include <utility>
#include <vector>
#include <string>
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "infra/storage/StorageManager.h"
#include <iostream>
#include <sqlite3.h>
#include "infra/storage/VectorSerializer.h"
#include "infra/storage/TimeUtils.h"
#include <cmath>
#include <algorithm>

bool ResourceMemoryManager::add_resource_memory(const ResourceMemoryItem& item) {
    std::cout << "\n=== 添加资源记忆开始 ===" << std::endl;
    std::cout << "用户ID: " << item.user_id << std::endl;
    std::cout << "Agent ID: " << item.agent_id << std::endl;
    std::cout << "会话ID: " << item.session_id << std::endl;
    std::cout << "路径: " << item.path << std::endl;
    std::cout << "描述: " << item.description << std::endl;
    std::cout << "资源类型: " << item.resource_type << std::endl;
    std::cout << "大小: " << item.size << std::endl;
    std::cout << "Embedding大小: " << item.embedding.size() << " 字节" << std::endl;
    
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "INSERT INTO resource_memory (user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (!storage_.executeQuery(query, stmt)) {
        std::cout << "=== 添加资源记忆结束(失败) ===" << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_int(stmt, 3, item.session_id);
    sqlite3_bind_text(stmt, 4, item.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, item.resource_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, item.size);
    sqlite3_bind_text(stmt, 8, item.metadata.c_str(), -1, SQLITE_STATIC);
    
    // 绑定embedding字段（BLOB类型）
    if (!item.embedding.empty()) {
        sqlite3_bind_blob(stmt, 9, item.embedding.data(), item.embedding.size(), SQLITE_STATIC);
        std::cout << "绑定embedding数据，大小: " << item.embedding.size() << " 字节" << std::endl;
    } else {
        sqlite3_bind_null(stmt, 9);
        std::cout << "绑定空的embedding数据" << std::endl;
    }
    
    sqlite3_bind_text(stmt, 10, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, currentTime.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to add resource memory: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        std::cout << "=== 添加资源记忆结束(失败) ===" << std::endl;
        return false;
    }
    
    std::cout << "=== 添加资源记忆结束(成功) ===" << std::endl;
    return true;
}

bool ResourceMemoryManager::update_resource_memory(int id, const ResourceMemoryItem& item) {
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "UPDATE resource_memory SET user_id = ?, agent_id = ?, session_id = ?, path = ?, description = ?, resource_type = ?, size = ?, metadata = ?, updated_at = ? WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_int(stmt, 3, item.session_id);
    sqlite3_bind_text(stmt, 4, item.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, item.resource_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, item.size);
    sqlite3_bind_text(stmt, 8, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 10, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

bool ResourceMemoryManager::delete_resource_memory(int id) {
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM resource_memory WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

ResourceMemoryManager::ResourceMemoryItem ResourceMemoryManager::get_last_resource_memory(int user_id) {
    ResourceMemoryItem item;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT 1;";
    
    if (!storage_.executeQuery(sql, stmt)) {
        return item;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.size = sqlite3_column_int(stmt, 7);
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 9);
        int blobSize = sqlite3_column_bytes(stmt, 9);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
    }
    
    sqlite3_finalize(stmt);
    return item;
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::list_resource_memories(int user_id, int limit) {
    std::vector<ResourceMemoryItem> results;
    
    sqlite3_stmt* stmt;
    std::string sql;
    
    if (limit > 0) {
        sql = "SELECT id, user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    } else {
        sql = "SELECT id, user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC;";
    }
    
    if (!storage_.executeQuery(sql, stmt)) {
        return results;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.size = sqlite3_column_int(stmt, 7);
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 9);
        int blobSize = sqlite3_column_bytes(stmt, 9);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        results.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::search_resource_memories(int user_id, const std::string& query, int limit) {
    std::vector<ResourceMemoryItem> results;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at FROM resource_memory WHERE user_id = ? AND (path LIKE ? OR description LIKE ?) ORDER BY created_at DESC LIMIT ?;";
    
    if (!storage_.executeQuery(sql, stmt)) {
        return results;
    }
    
    std::string searchPattern = "%" + query + "%";
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.size = sqlite3_column_int(stmt, 7);
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 9);
        int blobSize = sqlite3_column_bytes(stmt, 9);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        results.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

ResourceMemoryManager::ResourceMemoryItem ResourceMemoryManager::get_resource_memory(int id) {
    ResourceMemoryItem item;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, session_id, path, description, resource_type, size, metadata, embedding, created_at, updated_at FROM resource_memory WHERE id = ?;";
    
    if (!storage_.executeQuery(sql, stmt)) {
        return item;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.size = sqlite3_column_int(stmt, 7);
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 9);
        int blobSize = sqlite3_column_bytes(stmt, 9);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
    }
    
    sqlite3_finalize(stmt);
    return item;
}

bool ResourceMemoryManager::save_resource_memory_embedding(int memory_id, const std::vector<float>& embedding) {
    sqlite3_stmt* stmt;
    std::string query = "UPDATE resource_memory SET embedding = ?, updated_at = datetime('now') WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    // 将vector<float>序列化为二进制数据
    if (!embedding.empty()) {
        std::string serializedEmbedding(reinterpret_cast<const char*>(embedding.data()), 
                                       embedding.size() * sizeof(float));
        sqlite3_bind_blob(stmt, 1, serializedEmbedding.data(), serializedEmbedding.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    
    sqlite3_bind_int(stmt, 2, memory_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

bool ResourceMemoryManager::saveEmbedding(int memory_id, const std::vector<float>& embedding) {
    return save_resource_memory_embedding(memory_id, embedding);
}

std::vector<float> ResourceMemoryManager::getEmbedding(int resourceId) {
    sqlite3_stmt* stmt;
    std::string query = "SELECT embedding FROM resource_memory WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return std::vector<float>();
    }
    
    sqlite3_bind_int(stmt, 1, resourceId);
    
    std::vector<float> embedding;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 从BLOB字段读取embedding数据
        const void* blobData = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        
        if (blobData && blobSize > 0) {
            // 将二进制数据转换回vector<float>
            const float* floatData = static_cast<const float*>(blobData);
            size_t floatCount = blobSize / sizeof(float);
            embedding = std::vector<float>(floatData, floatData + floatCount);
        }
    }
    
    sqlite3_finalize(stmt);
    return embedding;
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK) {
    std::vector<ResourceMemoryItem> results;
    
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return results;
    }
    
    // 验证查询embedding是否为空
    if (queryEmbedding.empty()) {
        std::cerr << "Empty query embedding provided" << std::endl;
        return results;
    }
    
    // 简化实现：获取所有用户记忆并计算相似度
    auto allMemories = list_resource_memories(user_id, 0); // 获取所有记忆
    
    // 存储带相似度的记忆
    std::vector<std::pair<ResourceMemoryItem, float>> scoredMemories;
    
    for (const auto& memory : allMemories) {
        // 获取记忆的embedding
        auto memoryEmbedding = getEmbedding(memory.id);
        
        if (!memoryEmbedding.empty() && memoryEmbedding.size() == queryEmbedding.size()) {
            // 计算余弦相似度
            float dotProduct = 0.0f;
            float normQuery = 0.0f;
            float normMemory = 0.0f;
            
            for (size_t i = 0; i < queryEmbedding.size(); ++i) {
                dotProduct += queryEmbedding[i] * memoryEmbedding[i];
                normQuery += queryEmbedding[i] * queryEmbedding[i];
                normMemory += memoryEmbedding[i] * memoryEmbedding[i];
            }
            
            float similarity = 0.0f;
            if (normQuery > 0.0f && normMemory > 0.0f) {
                similarity = dotProduct / (std::sqrt(normQuery) * std::sqrt(normMemory));
            }
            
            scoredMemories.push_back({memory, similarity});
        }
    }
    
    // 按相似度排序
    std::sort(scoredMemories.begin(), scoredMemories.end(),
              [](const std::pair<ResourceMemoryItem, float>& a, const std::pair<ResourceMemoryItem, float>& b) {
                  return a.second > b.second;
              });
    
    // 取前topK个结果
    int count = 0;
    for (const auto& pair : scoredMemories) {
        if (count >= topK) break;
        results.push_back(pair.first);
        count++;
    }
    
    return results;
}