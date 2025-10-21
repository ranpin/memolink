#include "RecallMemoryManager.h"
#include "../core/StorageManager.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include "../../utils/VectorSerializer.h"
#include "../../utils/TimeUtils.h"

// 修复add_recall_memory方法
bool RecallMemoryManager::add_recall_memory(const RecallMemoryItem& item) {
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "INSERT INTO recall_memory (user_id, agent_id, session_id, message_type, content, metadata, created_at) VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_int(stmt, 3, item.session_id);
    sqlite3_bind_text(stmt, 4, item.message_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, currentTime.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    // 获取插入的记录ID
    const_cast<RecallMemoryItem&>(item).id = sqlite3_last_insert_rowid(storage_.getDatabase());
    return true;
}

// 修复update_recall_memory方法
bool RecallMemoryManager::update_recall_memory(int id, const RecallMemoryItem& item) {
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "UPDATE recall_memory SET user_id = ?, agent_id = ?, session_id = ?, message_type = ?, content = ?, metadata = ?, updated_at = ? WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, item.user_id);
    sqlite3_bind_int(stmt, 2, item.agent_id);
    sqlite3_bind_int(stmt, 3, item.session_id);
    sqlite3_bind_text(stmt, 4, item.message_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

// 修复delete_recall_memory方法
bool RecallMemoryManager::delete_recall_memory(int id) {
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM recall_memory WHERE id = ?;";

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

// 修复list_recall_memories方法
std::vector<RecallMemoryManager::RecallMemoryItem> RecallMemoryManager::list_recall_memories(int user_id, int limit) {
    std::vector<RecallMemoryItem> messages;

    sqlite3_stmt* stmt;
    std::string query;
    
    // 如果limit为0，则不限制返回数量
    if (limit == 0) {
        query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC;";
    } else {
        query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    }

    if (!storage_.executeQuery(query, stmt)) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    
    // 如果limit为0，则不需要绑定limit参数
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }

    int stepResult;
    while ((stepResult = sqlite3_step(stmt)) == SQLITE_ROW) {
        RecallMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.message_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.embedding = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        messages.push_back(item);
    }

    if (stepResult != SQLITE_DONE) {
        std::cout << "SQLite step error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return messages;
}

// 修复search_recall_memories方法
std::vector<RecallMemoryManager::RecallMemoryItem> RecallMemoryManager::search_recall_memories(int user_id, const std::string& query) {
    std::vector<RecallMemoryItem> messages;

    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at FROM recall_memory WHERE user_id = ? AND (content LIKE ? OR metadata LIKE ?);";

    if (!storage_.executeQuery(sql, stmt)) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    std::string filterPattern = "%" + query + "%";
    sqlite3_bind_text(stmt, 2, filterPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, filterPattern.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RecallMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        item.message_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.embedding = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        item.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        item.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        messages.push_back(item);
    }

    sqlite3_finalize(stmt);
    return messages;
}

// 修复saveEmbedding方法
bool RecallMemoryManager::saveEmbedding(int memoryId, const std::vector<float>& embedding) {
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "UPDATE recall_memory SET embedding = ? WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }

    // 使用VectorSerializer序列化向量
    std::string serializedEmbedding = VectorSerializer::serialize(embedding);

    // 绑定BLOB数据
    sqlite3_bind_blob(stmt, 1, serializedEmbedding.data(), serializedEmbedding.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, memoryId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to update embedding: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }

    return true;
}

// 修复getEmbedding方法
std::vector<float> RecallMemoryManager::getEmbedding(int memoryId) {
    std::vector<float> embedding;
    
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        return embedding;
    }

    sqlite3_stmt* stmt;
    std::string query = "SELECT embedding FROM recall_memory WHERE id = ?;";

    if (!storage_.executeQuery(query, stmt)) {
        return embedding;
    }

    sqlite3_bind_int(stmt, 1, memoryId);

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

// 修复searchByEmbedding方法
std::vector<RecallMemoryManager::RecallMemoryItem> RecallMemoryManager::searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK) {
    std::vector<RecallMemoryItem> results;
    
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
    auto allMemories = list_recall_memories(user_id, 0); // 获取所有记忆
    
    // 存储带相似度的记忆
    std::vector<std::pair<RecallMemoryItem, float>> scoredMemories;
    
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
              [](const std::pair<RecallMemoryItem, float>& a, const std::pair<RecallMemoryItem, float>& b) {
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
