#include "RecallMemoryManager.h"
#include "../core/StorageManager.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include "../../utils/VectorSerializer.h"
#include "../../utils/TimeUtils.h"

// 修复add_recall_memory方法
bool RecallMemoryManager::add_recall_memory(const RecallMemoryItem& item) {
    std::cout << "\n=== 添加回忆记忆开始 ===" << std::endl;
    std::cout << "用户ID: " << item.user_id << std::endl;
    std::cout << "Agent ID: " << item.agent_id << std::endl;
    std::cout << "会话ID: " << item.session_id << std::endl;
    std::cout << "消息类型: " << item.message_type << std::endl;
    std::cout << "内容: " << item.content << std::endl;
    
    // 确保数据库连接有效
    if (!storage_.getDatabase()) {
        std::cerr << "Database not initialized" << std::endl;
        std::cout << "=== 添加回忆记忆结束(失败) ===" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "INSERT INTO recall_memory (user_id, agent_id, session_id, message_type, content, metadata, created_at) VALUES (?, ?, ?, ?, ?, ?, ?);";

    if (!storage_.executeQuery(query, stmt)) {
        std::cout << "=== 添加回忆记忆结束(失败) ===" << std::endl;
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
        std::cout << "=== 添加回忆记忆结束(失败) ===" << std::endl;
        return false;
    }

    // 获取插入的记录ID
    const_cast<RecallMemoryItem&>(item).id = sqlite3_last_insert_rowid(storage_.getDatabase());
    std::cout << "插入记录ID: " << item.id << std::endl;
    std::cout << "=== 添加回忆记忆结束(成功) ===" << std::endl;
    return true;
}

// 修复list_recall_memories方法
std::vector<RecallMemoryManager::RecallMemoryItem> RecallMemoryManager::list_recall_memories(int user_id, int limit) {
    std::cout << "[RecallMemoryManager] list_recall_memories 开始执行..." << std::endl;
    std::cout << "[RecallMemoryManager] user_id: " << user_id << ", limit: " << limit << std::endl;
    
    std::vector<RecallMemoryItem> messages;
    
    sqlite3_stmt* stmt;
    std::string query;
    
    // 如果limit为0，则不限制返回数量
    if (limit == 0) {
        query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC;";
    } else {
        query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    }
    
    std::cout << "[RecallMemoryManager] 准备SQL查询: " << query << std::endl;

    if (!storage_.executeQuery(query, stmt)) {
        std::cout << "[RecallMemoryManager] SQL查询准备失败" << std::endl;
        return messages;
    }
    
    std::cout << "[RecallMemoryManager] SQL查询准备成功，绑定参数..." << std::endl;

    sqlite3_bind_int(stmt, 1, user_id);
    
    // 如果limit为0，则不需要绑定limit参数
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }
    
    std::cout << "[RecallMemoryManager] 参数绑定完成，开始执行查询..." << std::endl;

    int stepResult;
    int recordCount = 0;
    while ((stepResult = sqlite3_step(stmt)) == SQLITE_ROW) {
        recordCount++;
        std::cout << "[RecallMemoryManager] 处理第 " << recordCount << " 条记录..." << std::endl;
        
        RecallMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        
        // 安全地处理文本字段
        const char* message_type_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.message_type = message_type_text ? message_type_text : "";
        
        const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.content = content_text ? content_text : "";
        
        const char* metadata_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.metadata = metadata_text ? metadata_text : "";
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 7);
        int blobSize = sqlite3_column_bytes(stmt, 7);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        } else {
            item.embedding = "";
        }
        
        const char* created_at_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        item.created_at = created_at_text ? created_at_text : "";
        
        const char* updated_at_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.updated_at = updated_at_text ? updated_at_text : "";
        
        messages.push_back(item);
        std::cout << "[RecallMemoryManager] 第 " << recordCount << " 条记录处理完成" << std::endl;
    }

    if (stepResult != SQLITE_DONE) {
        std::cout << "SQLite step error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
    }
    
    std::cout << "[RecallMemoryManager] 查询执行完成，共处理 " << recordCount << " 条记录" << std::endl;

    sqlite3_finalize(stmt);
    std::cout << "[RecallMemoryManager] list_recall_memories 执行完成，返回 " << messages.size() << " 条记录" << std::endl;
    return messages;
}

// 添加新方法：查询用户最近几次会话的历史记录
std::vector<RecallMemoryManager::RecallMemoryItem> RecallMemoryManager::list_recent_sessions_memories(int user_id, int session_limit, int message_limit) {
    std::cout << "[RecallMemoryManager] list_recent_sessions_memories 开始执行..." << std::endl;
    std::cout << "[RecallMemoryManager] user_id: " << user_id << ", session_limit: " << session_limit << ", message_limit: " << message_limit << std::endl;
    
    std::vector<RecallMemoryItem> messages;
    
    // 首先获取用户最近的几个会话ID
    sqlite3_stmt* session_stmt;
    std::string session_query = "SELECT id FROM session WHERE user_id = ? ORDER BY id DESC LIMIT ?;";
    std::cout << "[RecallMemoryManager] 准备获取最近会话的SQL查询: " << session_query << std::endl;
    
    if (!storage_.executeQuery(session_query, session_stmt)) {
        std::cout << "[RecallMemoryManager] 获取最近会话SQL查询准备失败" << std::endl;
        return messages;
    }
    
    std::cout << "[RecallMemoryManager] 获取最近会话SQL查询准备成功，绑定参数..." << std::endl;
    sqlite3_bind_int(session_stmt, 1, user_id);
    sqlite3_bind_int(session_stmt, 2, session_limit);
    
    std::cout << "[RecallMemoryManager] 参数绑定完成，开始执行查询..." << std::endl;
    
    std::vector<int> session_ids;
    int session_count = 0;
    int stepResult;
    while ((stepResult = sqlite3_step(session_stmt)) == SQLITE_ROW) {
        session_count++;
        int session_id = sqlite3_column_int(session_stmt, 0);
        session_ids.push_back(session_id);
        std::cout << "[RecallMemoryManager] 获取到第 " << session_count << " 个会话ID: " << session_id << std::endl;
    }
    
    if (stepResult != SQLITE_DONE) {
        std::cout << "SQLite step error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
    }
    
    sqlite3_finalize(session_stmt);
    std::cout << "[RecallMemoryManager] 获取最近会话查询执行完成，共获取 " << session_count << " 个会话" << std::endl;
    
    if (session_ids.empty()) {
        std::cout << "[RecallMemoryManager] 未找到任何会话" << std::endl;
        return messages;
    }
    
    // 构建IN查询语句
    std::string in_clause = "";
    for (size_t i = 0; i < session_ids.size(); ++i) {
        if (i > 0) in_clause += ",";
        in_clause += std::to_string(session_ids[i]);
    }
    
    // 查询这些会话中的回忆记忆
    sqlite3_stmt* memory_stmt;
    std::string memory_query;
    
    if (message_limit == 0) {
        memory_query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at "
                      "FROM recall_memory WHERE user_id = ? AND session_id IN (" + in_clause + ") "
                      "ORDER BY created_at DESC;";
    } else {
        memory_query = "SELECT id, user_id, agent_id, session_id, message_type, content, metadata, embedding, created_at, updated_at "
                      "FROM recall_memory WHERE user_id = ? AND session_id IN (" + in_clause + ") "
                      "ORDER BY created_at DESC LIMIT ?;";
    }
    
    std::cout << "[RecallMemoryManager] 准备获取会话记忆的SQL查询: " << memory_query << std::endl;
    
    if (!storage_.executeQuery(memory_query, memory_stmt)) {
        std::cout << "[RecallMemoryManager] 获取会话记忆SQL查询准备失败" << std::endl;
        return messages;
    }
    
    std::cout << "[RecallMemoryManager] 获取会话记忆SQL查询准备成功，绑定参数..." << std::endl;
    sqlite3_bind_int(memory_stmt, 1, user_id);
    
    if (message_limit > 0) {
        sqlite3_bind_int(memory_stmt, 2, message_limit);
    }
    
    std::cout << "[RecallMemoryManager] 参数绑定完成，开始执行查询..." << std::endl;
    
    int memory_count = 0;
    while ((stepResult = sqlite3_step(memory_stmt)) == SQLITE_ROW) {
        memory_count++;
        std::cout << "[RecallMemoryManager] 处理第 " << memory_count << " 条记忆记录..." << std::endl;
        
        RecallMemoryItem item;
        item.id = sqlite3_column_int(memory_stmt, 0);
        item.user_id = sqlite3_column_int(memory_stmt, 1);
        item.agent_id = sqlite3_column_int(memory_stmt, 2);
        item.session_id = sqlite3_column_int(memory_stmt, 3);
        
        // 安全地处理文本字段
        const char* message_type_text = reinterpret_cast<const char*>(sqlite3_column_text(memory_stmt, 4));
        item.message_type = message_type_text ? message_type_text : "";
        
        const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(memory_stmt, 5));
        item.content = content_text ? content_text : "";
        
        const char* metadata_text = reinterpret_cast<const char*>(sqlite3_column_text(memory_stmt, 6));
        item.metadata = metadata_text ? metadata_text : "";
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(memory_stmt, 7);
        int blobSize = sqlite3_column_bytes(memory_stmt, 7);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        } else {
            item.embedding = "";
        }
        
        const char* created_at_text = reinterpret_cast<const char*>(sqlite3_column_text(memory_stmt, 8));
        item.created_at = created_at_text ? created_at_text : "";
        
        const char* updated_at_text = reinterpret_cast<const char*>(sqlite3_column_text(memory_stmt, 9));
        item.updated_at = updated_at_text ? updated_at_text : "";
        
        messages.push_back(item);
        std::cout << "[RecallMemoryManager] 第 " << memory_count << " 条记忆记录处理完成" << std::endl;
    }
    
    if (stepResult != SQLITE_DONE) {
        std::cout << "SQLite step error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
    }
    
    std::cout << "[RecallMemoryManager] 获取会话记忆查询执行完成，共处理 " << memory_count << " 条记录" << std::endl;
    
    sqlite3_finalize(memory_stmt);
    std::cout << "[RecallMemoryManager] list_recent_sessions_memories 执行完成，返回 " << messages.size() << " 条记录" << std::endl;
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

    std::string searchPattern = "%" + query + "%";
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_STATIC);

    int stepResult;
    while ((stepResult = sqlite3_step(stmt)) == SQLITE_ROW) {
        RecallMemoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = sqlite3_column_int(stmt, 1);
        item.agent_id = sqlite3_column_int(stmt, 2);
        item.session_id = sqlite3_column_int(stmt, 3);
        
        const char* message_type_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.message_type = message_type_text ? message_type_text : "";
        
        const char* content_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        item.content = content_text ? content_text : "";
        
        const char* metadata_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        item.metadata = metadata_text ? metadata_text : "";
        
        // 处理BLOB类型的embedding字段
        const void* blobData = sqlite3_column_blob(stmt, 7);
        int blobSize = sqlite3_column_bytes(stmt, 7);
        if (blobData && blobSize > 0) {
            std::string serializedData(static_cast<const char*>(blobData), blobSize);
            item.embedding = serializedData;
        } else {
            item.embedding = "";
        }
        
        const char* created_at_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        item.created_at = created_at_text ? created_at_text : "";
        
        const char* updated_at_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        item.updated_at = updated_at_text ? updated_at_text : "";
        
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
    // 使用BLOB类型存储embedding
    std::string query = "UPDATE recall_memory SET embedding = ?, updated_at = datetime('now') WHERE id = ?;";
    
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
    
    sqlite3_bind_int(stmt, 2, memoryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

std::vector<float> RecallMemoryManager::getEmbedding(int memoryId) {
    sqlite3_stmt* stmt;
    std::string query = "SELECT embedding FROM recall_memory WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return std::vector<float>();
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    
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
