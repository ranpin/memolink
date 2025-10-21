#include "ResourceMemoryManager.h"
#include "../core/StorageManager.h"
#include "../../utils/TimeUtils.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

bool ResourceMemoryManager::add_resource_memory(const ResourceMemoryItem& item) {
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "INSERT INTO resource_memory (user_id, agent_id, filename, content, resource_type, summary, size, tree_path, metadata, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

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

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    // 获取插入的记录ID
    const_cast<ResourceMemoryItem&>(item).id = sqlite3_last_insert_rowid(storage_.getDatabase());
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
    std::vector<ResourceMemoryItem> resources;

    sqlite3_stmt* stmt;
    std::string query;
    
    // 如果limit为0，则不限制返回数量
    if (limit == 0) {
        query = "SELECT id, user_id, agent_id, filename, content, resource_type, summary, size, tree_path, metadata, embedding, last_accessed, created_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC;";
    } else {
        query = "SELECT id, user_id, agent_id, filename, content, resource_type, summary, size, tree_path, metadata, embedding, last_accessed, created_at FROM resource_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    }

    if (!storage_.executeQuery(query, stmt)) {
        return resources;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    
    // 如果limit为0，则不需要绑定limit参数
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)); // filename映射到title
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)); // tree_path映射到path
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.embedding = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        resources.push_back(item);
    }

    sqlite3_finalize(stmt);
    return resources;
}

// This method was removed as it's not in the new interface

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryManager::search_resource_memories(int user_id, const std::string& query) {
    std::vector<ResourceMemoryItem> resources;

    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, title, content, resource_type, path, metadata, embedding, created_at FROM resource_memory WHERE user_id = ? AND (title LIKE ? OR content LIKE ?);";

    if (!storage_.executeQuery(sql, stmt)) {
        return resources;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    std::string filterPattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 2, filterPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, filterPattern.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResourceMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.resource_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.embedding = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        resources.push_back(item);
    }

    sqlite3_finalize(stmt);
    return resources;
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

    // 将向量转换为逗号分隔的字符串
    std::ostringstream oss;
    for (size_t i = 0; i < embedding.size(); ++i) {
        if (i > 0) oss << ",";
        oss << embedding[i];
    }
    std::string embeddingStr = oss.str();

    sqlite3_bind_text(stmt, 1, embeddingStr.c_str(), -1, SQLITE_STATIC);
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
        const char* embeddingStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (embeddingStr) {
            std::string str(embeddingStr);
            // 检查是否为空字符串
            if (!str.empty()) {
                std::istringstream iss(str);
                std::string token;
                while (std::getline(iss, token, ',')) {
                    try {
                        embedding.push_back(std::stof(token));
                    } catch (const std::exception& e) {
                        // 忽略转换错误
                        std::cerr << "Error parsing embedding value: " << token << std::endl;
                    }
                }
            }
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
