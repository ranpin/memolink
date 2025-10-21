#include "MultimodalDataManager.h"
#include "../utils/TimeUtils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

MultimodalDataManager::MultimodalDataManager(const std::string& dbPath) : storage_(dbPath) {
    // 检查数据库路径是否有效
    if (dbPath.empty()) {
        std::cerr << "Error: Database path is empty" << std::endl;
        return;
    }
    
    if (!storage_.initialize()) {
        std::cerr << "Failed to initialize storage manager with path: " << dbPath << std::endl;
        return;
    }
    
    if (!createTableIfNotExists()) {
        std::cerr << "Failed to create table in database: " << dbPath << std::endl;
    }
}

MultimodalDataManager::~MultimodalDataManager() = default;

bool MultimodalDataManager::createTableIfNotExists() {
    std::string query = R"(
        CREATE TABLE IF NOT EXISTS multimodal_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            data_type TEXT NOT NULL,
            file_path TEXT NOT NULL,
            description TEXT,
            emotions TEXT,
            transcription TEXT,
            embedding TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            metadata TEXT
        );
    )";
    
    return storage_.executeNonQuery(query);
}

bool MultimodalDataManager::saveMultimodalData(int userId, const std::string& dataType, 
                                              const std::string& filePath, const std::string& description,
                                              const std::string& emotions, const std::string& transcription,
                                              const std::vector<float>& embedding, const std::string& metadata) {
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO multimodal_data (user_id, data_type, file_path, description, emotions, transcription, embedding, metadata, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
    // 检查数据库连接是否有效
    if (!storage_.getDatabase()) {
        std::cerr << "数据库未初始化" << std::endl;
        return false;
    }
    
    if (!storage_.executeQuery(query, stmt)) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    // 将embedding向量转换为字符串
    std::string embeddingStr;
    for (size_t i = 0; i < embedding.size(); ++i) {
        if (i > 0) embeddingStr += ",";
        embeddingStr += std::to_string(embedding[i]);
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, dataType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, filePath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, emotions.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, transcription.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, embeddingStr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, TimeUtils::getCurrentTimestamp().c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<MultimodalDataItem> MultimodalDataManager::getMultimodalDataByUserId(int userId, int limit) {
    std::vector<MultimodalDataItem> dataList;
    
    // 检查数据库连接是否有效
    if (!storage_.getDatabase()) {
        std::cerr << "数据库未初始化" << std::endl;
        return dataList;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, metadata, timestamp FROM multimodal_data WHERE user_id = ? ORDER BY timestamp DESC LIMIT ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return dataList;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dataList.push_back(rowToMultimodalDataItem(stmt));
    }
    
    sqlite3_finalize(stmt);
    return dataList;
}

std::vector<MultimodalDataItem> MultimodalDataManager::searchMultimodalData(int userId, const std::string& query, int limit) {
    std::vector<MultimodalDataItem> results;
    
    // 验证数据库连接
    if (!storage_.getDatabase()) {
        std::cerr << "Error: Cannot search data - database not initialized" << std::endl;
        return results;
    }
    
    if (query.empty()) {
        std::cerr << "Warning: Search query is empty, returning empty results" << std::endl;
        return results;
    }
    
    sqlite3_stmt* stmt;
    std::string sqlQuery = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, timestamp, metadata FROM multimodal_data WHERE user_id = ? AND (description LIKE ? OR emotions LIKE ? OR transcription LIKE ?) ORDER BY timestamp DESC LIMIT ?;";
    
    if (!storage_.executeQuery(sqlQuery, stmt)) {
        std::cerr << "Failed to prepare search query: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return results;
    }
    
    std::string searchPattern = "%" + query + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, searchPattern.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(rowToMultimodalDataItem(stmt));
    }
    
    sqlite3_finalize(stmt);
    return results;
}

bool MultimodalDataManager::saveMultimodalEmbedding(int dataId, const std::vector<float>& embedding) {
    // 为了保持向后兼容性，我们仍然将embedding保存到multimodal_data表中
    // 但主要的embedding存储将直接在resource_memory表中进行
    
    std::string embeddingStr;
    for (size_t i = 0; i < embedding.size(); ++i) {
        embeddingStr += std::to_string(embedding[i]);
        if (i < embedding.size() - 1) {
            embeddingStr += ",";
        }
    }
    
    std::string updateSql = "UPDATE multimodal_data SET embedding = ? WHERE id = ?;";
    sqlite3_stmt* updateStmt;
    
    if (!storage_.executeQuery(updateSql, updateStmt)) {
        std::cerr << "无法准备更新语句" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(updateStmt, 1, embeddingStr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(updateStmt, 2, dataId);
    
    int rc = sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "更新embedding失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    // 同时将embedding保存到resource_memory表中，实现您提出的方案
    // 首先获取multimodal_data记录以获取相关信息
    std::string selectSql = "SELECT user_id, data_type, file_path, description FROM multimodal_data WHERE id = ?;";
    sqlite3_stmt* selectStmt;
    
    if (!storage_.executeQuery(selectSql, selectStmt)) {
        std::cerr << "无法准备查询语句" << std::endl;
        return false;
    }
    
    sqlite3_bind_int(selectStmt, 1, dataId);
    
    rc = sqlite3_step(selectStmt);
    if (rc == SQLITE_ROW) {
        int user_id = sqlite3_column_int(selectStmt, 0);
        std::string data_type = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 1));
        std::string file_path = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 2));
        std::string description = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 3));
        
        sqlite3_finalize(selectStmt);
        
        // 更新resource_memory表中的embedding
        std::string updateResourceSql = "UPDATE resource_memory SET embedding = ? WHERE path = ? AND user_id = ?;";
        sqlite3_stmt* updateResourceStmt;
        
        if (!storage_.executeQuery(updateResourceSql, updateResourceStmt)) {
            std::cerr << "无法准备更新resource_memory语句" << std::endl;
            return false;
        }
        
        sqlite3_bind_text(updateResourceStmt, 1, embeddingStr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(updateResourceStmt, 2, file_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(updateResourceStmt, 3, user_id);
        
        rc = sqlite3_step(updateResourceStmt);
        sqlite3_finalize(updateResourceStmt);
        
        if (rc != SQLITE_DONE) {
            std::cerr << "更新resource_memory中的embedding失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
            return false;
        }
    } else {
        sqlite3_finalize(selectStmt);
        std::cerr << "未找到ID为" << dataId << "的multimodal_data记录" << std::endl;
        return false;
    }
    
    return true;
}

std::vector<MultimodalDataItem> MultimodalDataManager::searchMultimodalByEmbedding(const std::vector<float>& queryEmbedding, int limit) {
    std::vector<MultimodalDataItem> results;
    
    if (queryEmbedding.empty()) {
        return results;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, timestamp, metadata FROM multimodal_data WHERE embedding IS NOT NULL;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return results;
    }
    
    std::vector<std::pair<MultimodalDataItem, float>> scoredResults;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MultimodalDataItem item = rowToMultimodalDataItem(stmt);
        
        // 计算相似度（余弦相似度）
        float similarity = 0.0f;
        if (item.embedding.size() == queryEmbedding.size() && !item.embedding.empty()) {
            float dot_product = 0.0f;
            float norm_a = 0.0f;
            float norm_b = 0.0f;
            
            for (size_t i = 0; i < queryEmbedding.size(); ++i) {
                dot_product += queryEmbedding[i] * item.embedding[i];
                norm_a += queryEmbedding[i] * queryEmbedding[i];
                norm_b += item.embedding[i] * item.embedding[i];
            }
            
            if (norm_a != 0.0f && norm_b != 0.0f) {
                similarity = dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
            }
        }
        
        scoredResults.push_back({item, similarity});
    }
    
    sqlite3_finalize(stmt);
    
    // 按相似度降序排序
    std::sort(scoredResults.begin(), scoredResults.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 限制返回数量
    for (int i = 0; i < limit && i < static_cast<int>(scoredResults.size()); ++i) {
        results.push_back(scoredResults[i].first);
    }
    
    return results;
}

std::vector<float> MultimodalDataManager::generateTextEmbedding(const std::string& text) {
    // 这里应该调用实际的文本嵌入模型（如Qwen）
    // 目前返回一个占位符向量
    std::vector<float> embedding(768, 0.1f); // 768维向量，填充值为0.1
    return embedding;
}

std::vector<float> MultimodalDataManager::generateImageEmbedding(const std::string& imagePath) {
    // 这里应该调用实际的图像嵌入模型（如CLIP）
    // 目前返回一个占位符向量
    std::vector<float> embedding(512, 0.2f); // 512维向量，填充值为0.2
    return embedding;
}

std::vector<float> MultimodalDataManager::generateAudioEmbedding(const std::string& audioPath) {
    // 这里应该调用实际的音频嵌入模型
    // 目前返回一个占位符向量
    std::vector<float> embedding(384, 0.3f); // 384维向量，填充值为0.3
    return embedding;
}

bool MultimodalDataManager::deleteMultimodalData(int id) {
    // 检查数据库连接是否有效
    if (!storage_.getDatabase()) {
        std::cerr << "数据库未初始化" << std::endl;
        return false;
    }
    
    std::string query = "DELETE FROM multimodal_data WHERE id = ?;";
    sqlite3_stmt* stmt;
    
    if (!storage_.executeQuery(query, stmt)) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
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

// 将数据库行转换为 MultimodalDataItem
MultimodalDataItem MultimodalDataManager::rowToMultimodalDataItem(sqlite3_stmt* stmt) {
    MultimodalDataItem item;
    item.id = sqlite3_column_int(stmt, 0);
    item.userId = sqlite3_column_int(stmt, 1);
    item.dataType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    item.filePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    item.emotions = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    item.transcription = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    
    // 解析embedding向量
    const char* embeddingStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    if (embeddingStr) {
        std::string embeddingString(embeddingStr);
        std::stringstream ss(embeddingString);
        std::string token;
        while (std::getline(ss, token, ',')) {
            try {
                item.embedding.push_back(std::stof(token));
            } catch (...) {
                // 忽略转换错误
            }
        }
    }
    
    item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    item.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    
    return item;
}