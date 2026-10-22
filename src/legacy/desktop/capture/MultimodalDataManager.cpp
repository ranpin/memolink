#include <utility>
#include <vector>
#include <string>
#include <memory>
#include "legacy/desktop/capture/MultimodalDataManager.h"
#include "infra/storage/TimeUtils.h"
#include "infra/storage/VectorSerializer.h"
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
    
    // 初始化CLIP处理器
    clipProcessor_ = std::make_unique<CLIPProcessor>();
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
            embedding BLOB,
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
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, dataType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, filePath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, emotions.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, transcription.c_str(), -1, SQLITE_STATIC);
    
    // 序列化embedding向量
    std::string serializedEmbedding = VectorSerializer::serialize(embedding);
    sqlite3_bind_blob(stmt, 7, serializedEmbedding.data(), static_cast<int>(serializedEmbedding.size()), SQLITE_STATIC);
    
    sqlite3_bind_text(stmt, 8, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, TimeUtils::getCurrentTimestamp().c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<MultimodalDataItem> MultimodalDataManager::getMultimodalDataByUserId(int userId, int limit) {
    std::vector<MultimodalDataItem> results;
    sqlite3_stmt* stmt;
    
    std::string query = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, timestamp, metadata FROM multimodal_data WHERE user_id = ? ORDER BY timestamp DESC LIMIT ?;";
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return results;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(rowToMultimodalDataItem(stmt));
    }
    
    sqlite3_finalize(stmt);
    
    return results;
}

std::vector<MultimodalDataItem> MultimodalDataManager::searchMultimodalData(int userId, const std::string& query, int limit) {
    std::vector<MultimodalDataItem> results;
    sqlite3_stmt* stmt;
    
    std::string sql = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, timestamp, metadata FROM multimodal_data WHERE user_id = ? AND (description LIKE ? OR transcription LIKE ?) ORDER BY timestamp DESC LIMIT ?;";
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return results;
    }
    
    std::string searchQuery = "%" + query + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchQuery.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchQuery.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, limit);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        results.push_back(rowToMultimodalDataItem(stmt));
    }
    
    sqlite3_finalize(stmt);
    
    return results;
}

bool MultimodalDataManager::saveMultimodalEmbedding(int dataId, const std::vector<float>& embedding) {
    sqlite3_stmt* stmt;
    std::string query = "UPDATE multimodal_data SET embedding = ? WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    // 序列化embedding向量
    std::string serializedEmbedding = VectorSerializer::serialize(embedding);
    sqlite3_bind_blob(stmt, 1, serializedEmbedding.data(), static_cast<int>(serializedEmbedding.size()), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, dataId);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<MultimodalDataItem> MultimodalDataManager::searchMultimodalByEmbedding(const std::vector<float>& queryEmbedding, int limit) {
    std::vector<MultimodalDataItem> results;
    // 简化的向量搜索实现
    // 在实际应用中，这里应该使用更高效的向量搜索算法或库
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, data_type, file_path, description, emotions, transcription, embedding, timestamp, metadata FROM multimodal_data WHERE embedding IS NOT NULL;";
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return results;
    }
    
    // 存储所有带embedding的数据项及其相似度
    std::vector<std::pair<MultimodalDataItem, float>> similarityResults;
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MultimodalDataItem item = rowToMultimodalDataItem(stmt);
        
        // 计算余弦相似度
        if (!item.embedding.empty() && !queryEmbedding.empty()) {
            // 确保向量维度相同
            size_t minSize = std::min(item.embedding.size(), queryEmbedding.size());
            if (minSize > 0) {
                float dotProduct = 0.0f;
                float normA = 0.0f;
                float normB = 0.0f;
                
                for (size_t i = 0; i < minSize; ++i) {
                    dotProduct += item.embedding[i] * queryEmbedding[i];
                    normA += item.embedding[i] * item.embedding[i];
                    normB += queryEmbedding[i] * queryEmbedding[i];
                }
                
                float similarity = 0.0f;
                if (normA > 0 && normB > 0) {
                    similarity = dotProduct / (std::sqrt(normA) * std::sqrt(normB));
                }
                
                similarityResults.emplace_back(item, similarity);
            }
        }
    }
    
    sqlite3_finalize(stmt);
    
    // 按相似度排序
    std::sort(similarityResults.begin(), similarityResults.end(),
              [](const std::pair<MultimodalDataItem, float>& a, const std::pair<MultimodalDataItem, float>& b) {
                  return a.second > b.second; // 降序排列
              });
    
    // 取前limit个结果
    for (size_t i = 0; i < std::min(static_cast<size_t>(limit), similarityResults.size()); ++i) {
        results.push_back(similarityResults[i].first);
    }
    
    return results;
}

std::vector<float> MultimodalDataManager::generateTextEmbedding(const std::string& text) {
    if (clipProcessor_) {
        return clipProcessor_->encodeText(text);
    }
    return std::vector<float>();
}

std::vector<float> MultimodalDataManager::generateImageEmbedding(const std::string& imagePath) {
    if (clipProcessor_) {
        return clipProcessor_->encodeImage(imagePath);
    }
    return std::vector<float>();
}

std::vector<float> MultimodalDataManager::generateAudioEmbedding(const std::string& audioPath) {
    // 音频embedding生成逻辑
    // 目前返回空向量，表示使用其他方式处理音频
    return std::vector<float>();
}

bool MultimodalDataManager::deleteMultimodalData(int id) {
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM multimodal_data WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(storage_.getDatabase(), query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

MultimodalDataItem MultimodalDataManager::rowToMultimodalDataItem(sqlite3_stmt* stmt) {
    MultimodalDataItem item;
    item.id = sqlite3_column_int(stmt, 0);
    item.userId = sqlite3_column_int(stmt, 1);
    item.dataType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    item.filePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    item.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    item.emotions = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    item.transcription = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    
    // 反序列化embedding向量
    const void* blob = sqlite3_column_blob(stmt, 7);
    int blobSize = sqlite3_column_bytes(stmt, 7);
    if (blob && blobSize > 0) {
        std::string serializedData(static_cast<const char*>(blob), blobSize);
        item.embedding = VectorSerializer::deserialize(serializedData);
    }
    
    item.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    
    return item;
}