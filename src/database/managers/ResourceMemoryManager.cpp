#include "ResourceMemoryManager.h"
#include "../core/StorageManager.h"
#include <iostream>
#include <sqlite3.h>
#include "../../utils/VectorSerializer.h"
#include "../../utils/TimeUtils.h"

bool ResourceMemoryManager::add_resource_memory(const ResourceMemoryItem& item) {
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "INSERT INTO resource_memory (user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, last_accessed, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_text(stmt, 3, item.filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, item.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, item.resource_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, item.summary.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, item.size);
    sqlite3_bind_text(stmt, 9, item.tree_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, item.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, currentTime.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to add resource memory: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

bool ResourceMemoryManager::update_resource_memory(int id, const ResourceMemoryItem& item) {
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "UPDATE resource_memory SET user_id = ?, agent_id = ?, filename = ?, content = ?, resource_type = ?, summary = ?, size = ?, tree_path = ?, metadata = ?, updated_at = ? WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_text(stmt, 3, item.title.c_str(), -1, SQLITE_STATIC); // title映射到filename
    sqlite3_bind_text(stmt, 4, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.resource_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, "", -1, SQLITE_STATIC); // summary字段
    sqlite3_bind_int(stmt, 7, 0); // size字段
    sqlite3_bind_text(stmt, 8, "", -1, SQLITE_STATIC); // tree_path字段
    sqlite3_bind_text(stmt, 9, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 11, id);

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

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::list_resource_memories(int user_id, int limit) {
    std::vector<ResourceMemoryItem> results;
    
    sqlite3_stmt* stmt;
    std::string sql;
    
    if (limit > 0) {
        sql = "SELECT id, user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, embedding, last_accessed, created_at, updated_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    } else {
        sql = "SELECT id, user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, embedding, last_accessed, created_at, updated_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC;";
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
        item.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.size = sqlite3_column_int(stmt, 8);
        item.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 12);
        int blobSize = sqlite3_column_bytes(stmt, 12);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.last_accessed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        
        results.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

// This method was removed as it's not in the new interface

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::search_resource_memories(int user_id, const std::string& query) {
    std::vector<ResourceMemoryItem> results;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, embedding, last_accessed, created_at, updated_at FROM resource_memory WHERE user_id = ? AND (title LIKE ? OR content LIKE ?) ORDER BY created_at DESC LIMIT 100;";
    
    if (!storage_.executeQuery(sql, stmt)) {
        return results;
    }
    
    std::string searchPattern = "%" + query + "%";
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.size = sqlite3_column_int(stmt, 8);
        item.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 12);
        int blobSize = sqlite3_column_bytes(stmt, 12);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.last_accessed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        
        results.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

bool ResourceMemoryManager::saveEmbedding(int resourceId, const std::vector<float>& embedding) {
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "UPDATE resource_memory SET embedding = ? WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    // 使用VectorSerializer序列化向量
    std::string serializedEmbedding = VectorSerializer::serialize(embedding);

    // 绑定BLOB数据
    sqlite3_bind_blob(stmt, 1, serializedEmbedding.data(), serializedEmbedding.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, resourceId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to update embedding: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

// 修复getEmbedding方法
std::vector<float> ResourceMemoryManager::getEmbedding(int resourceId) {
    std::vector<float> embedding;
    
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return embedding;
    }

    sqlite3_stmt* stmt;
    std::string query = "SELECT embedding FROM resource_memory WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return embedding;
    }

    sqlite3_bind_int(stmt, 1, resourceId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 从BLOB字段读取数据
        const void* blobData = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        
        if (blobData && blobSize > 0) {
            // 将BLOB数据转换为字符串
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            // 使用VectorSerializer反序列化向量
            embedding = VectorSerializer::deserialize(serializedData);
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

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::getResourcesByType(int user_id, const std::string& resourceType) {
    std::vector<ResourceMemoryItem> results;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, embedding, last_accessed, created_at, updated_at FROM resource_memory WHERE user_id = ? AND resource_type = ? ORDER BY created_at DESC;";
    
    if (!storage_.executeQuery(sql, stmt)) {
        return results;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, resourceType.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.size = sqlite3_column_int(stmt, 8);
        item.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 12);
        int blobSize = sqlite3_column_bytes(stmt, 12);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.last_accessed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        
        results.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return results;
}

ResourceMemoryManager::ResourceMemoryItem ResourceMemoryManager::getResourceById(int id) {
    ResourceMemoryItem item;
    item.id = -1; // 默认无效ID
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, filename, title, content, resource_type, summary, size, tree_path, path, metadata, embedding, last_accessed, created_at, updated_at FROM resource_memory WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return item;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.size = sqlite3_column_int(stmt, 8);
        item.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 12);
        int blobSize = sqlite3_column_bytes(stmt, 12);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        }
        
        item.last_accessed = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
    }
    
    sqlite3_finalize(stmt);
    return item;
}
