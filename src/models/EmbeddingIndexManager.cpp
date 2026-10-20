#include <utility>
#include <vector>
#include <string>
#include "EmbeddingIndexManager.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <algorithm>
#include <cmath>

bool EmbeddingIndexManager::addEmbedding(int memoryId, const std::string& memoryType, 
                                       const std::vector<float>& embedding, const std::string& metadata,
                                       const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO embedding_index (memory_id, memory_type, embedding, metadata) VALUES (?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    // 将嵌入向量转换为字符串
    std::ostringstream vectorStr;
    for (size_t i = 0; i < embedding.size(); ++i) {
        vectorStr << embedding[i];
        if (i < embedding.size() - 1) {
            vectorStr << ",";
        }
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    sqlite3_bind_text(stmt, 2, memoryType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, vectorStr.str().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, metadata.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool EmbeddingIndexManager::updateEmbedding(int id, const std::vector<float>& embedding, 
                                          const std::string& metadata, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "UPDATE embedding_index SET embedding = ?, metadata = ? WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    // 将嵌入向量转换为字符串
    std::ostringstream vectorStr;
    for (size_t i = 0; i < embedding.size(); ++i) {
        vectorStr << embedding[i];
        if (i < embedding.size() - 1) {
            vectorStr << ",";
        }
    }
    
    sqlite3_bind_text(stmt, 1, vectorStr.str().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool EmbeddingIndexManager::deleteEmbedding(int id, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM embedding_index WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<EmbeddingRecord> EmbeddingIndexManager::searchEmbeddings(const std::vector<float>& queryEmbedding, 
                                                                    int topK, const std::string& dbPath) {
    std::vector<EmbeddingRecord> results;
    
    // 基于SQLite的线性搜索和余弦相似度计算
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return results;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, memory_id, memory_type, embedding, metadata, created_at FROM embedding_index;";
    
    if (!storage.executeQuery(query, stmt)) {
        return results;
    }
    
    std::vector<std::pair<EmbeddingRecord, float>> scoredResults;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EmbeddingRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.memory_id = sqlite3_column_int(stmt, 1);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        // 解析向量数据
        std::string vectorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::stringstream ss(vectorStr);
        std::string item;
        while (std::getline(ss, item, ',')) {
            record.embedding.push_back(std::stof(item));
        }
        
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        // 计算相似度
        float similarity = 0.0f;
        if (record.embedding.size() == queryEmbedding.size() && !record.embedding.empty()) {
            float dot_product = 0.0f;
            float norm_a = 0.0f;
            float norm_b = 0.0f;
            
            for (size_t i = 0; i < queryEmbedding.size(); ++i) {
                dot_product += queryEmbedding[i] * record.embedding[i];
                norm_a += queryEmbedding[i] * queryEmbedding[i];
                norm_b += record.embedding[i] * record.embedding[i];
            }
            
            if (norm_a != 0.0f && norm_b != 0.0f) {
                similarity = dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
            }
        }
        
        scoredResults.push_back({record, similarity});
    }
    
    sqlite3_finalize(stmt);
    
    // 按相似度降序排序
    std::sort(scoredResults.begin(), scoredResults.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 限制返回数量
    for (int i = 0; i < topK && i < static_cast<int>(scoredResults.size()); ++i) {
        results.push_back(scoredResults[i].first);
    }
    
    return results;
}

EmbeddingRecord EmbeddingIndexManager::getEmbeddingById(int id, const std::string& dbPath) {
    EmbeddingRecord record;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return record;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, memory_id, memory_type, embedding, metadata, created_at FROM embedding_index WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.memory_id = sqlite3_column_int(stmt, 1);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        // 解析向量数据
        std::string vectorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::stringstream ss(vectorStr);
        std::string item;
        while (std::getline(ss, item, ',')) {
            record.embedding.push_back(std::stof(item));
        }
        
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

// 添加FAISS相关实现
bool EmbeddingIndexManager::initializeFAISSIndex(int dimension, const std::string& indexPath) {
    // 这里应该初始化FAISS索引，但目前仅作为占位符
    // 实际实现需要链接FAISS库
    std::cout << "FAISS index initialization requested with dimension: " << dimension 
              << " and path: " << indexPath << std::endl;
    return true;
}

bool EmbeddingIndexManager::saveFAISSIndex(const std::string& indexPath) {
    // 这里应该保存FAISS索引到文件，但目前仅作为占位符
    std::cout << "FAISS index save requested to path: " << indexPath << std::endl;
    return true;
}

bool EmbeddingIndexManager::loadFAISSIndex(const std::string& indexPath) {
    // 这里应该从文件加载FAISS索引，但目前仅作为占位符
    std::cout << "FAISS index load requested from path: " << indexPath << std::endl;
    return true;
}

std::vector<std::pair<int, float>> EmbeddingIndexManager::searchEmbeddingsFAISS(const std::vector<float>& queryEmbedding, int topK) {
    // 这里应该使用FAISS进行搜索，但目前仅作为占位符
    // 实际实现应该使用FAISS库进行近似最近邻搜索
    std::cout << "FAISS search requested with topK: " << topK << std::endl;
    return std::vector<std::pair<int, float>>(); // 返回空结果
}