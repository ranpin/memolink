#include "EmbeddingIndexManager.h"
#include "../utils/VectorSerializer.h"
#include "../utils/TimeUtils.h"
#include <sqlite3.h>
#include <iostream>
#include <cmath>
#include <algorithm>

// 辅助函数：创建 embedding 表（如果不存在）
static bool ensureEmbeddingTableExists(const std::string& dbPath) {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS embedding_index (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            memory_id INTEGER NOT NULL,
            memory_type TEXT NOT NULL,
            embedding BLOB NOT NULL,
            metadata TEXT,
            created_at TEXT NOT NULL
        )
    )";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager] 创建表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }
    
    sqlite3_close(db);
    return true;
}

// 辅助函数：计算余弦相似度
static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    if (normA == 0.0f || normB == 0.0f) return 0.0f;
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

bool EmbeddingIndexManager::addEmbedding(int memoryId, const std::string& memoryType, 
                                       const std::vector<float>& embedding, const std::string& metadata,
                                       const std::string& dbPath) {
    if (!ensureEmbeddingTableExists(dbPath)) {
        return false;
    }
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::addEmbedding] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // 序列化 embedding 向量
    std::string embeddingBlob = VectorSerializer::serialize(embedding);
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    const char* insertSQL = "INSERT INTO embedding_index (memory_id, memory_type, embedding, metadata, created_at) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    
    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::addEmbedding] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    sqlite3_bind_text(stmt, 2, memoryType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, embeddingBlob.data(), embeddingBlob.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, metadata.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    
    if (!success) {
        std::cerr << "[EmbeddingIndexManager::addEmbedding] 插入失败: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "[EmbeddingIndexManager::addEmbedding] ✓ 成功添加 embedding: memory_id=" << memoryId 
                  << ", type=" << memoryType << ", dimension=" << embedding.size() << std::endl;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}

bool EmbeddingIndexManager::updateEmbedding(int id, const std::vector<float>& embedding, 
                                          const std::string& metadata, const std::string& dbPath) {
    if (!ensureEmbeddingTableExists(dbPath)) {
        return false;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::updateEmbedding] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::string embeddingBlob = VectorSerializer::serialize(embedding);
    std::string timestamp = TimeUtils::getCurrentTimestamp();

    const char* updateSQL = "UPDATE embedding_index SET embedding = ?, metadata = ?, created_at = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    rc = sqlite3_prepare_v2(db, updateSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::updateEmbedding] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_blob(stmt, 1, embeddingBlob.data(), static_cast<int>(embeddingBlob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, metadata.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);

    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE && sqlite3_changes(db) > 0);

    if (!success) {
        std::cerr << "[EmbeddingIndexManager::updateEmbedding] 更新失败: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "[EmbeddingIndexManager::updateEmbedding] ✓ 成功更新 embedding, id=" << id
                  << ", dimension=" << embedding.size() << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}
bool EmbeddingIndexManager::deleteEmbedding(int id, const std::string& dbPath) {
    if (!ensureEmbeddingTableExists(dbPath)) {
        return false;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::deleteEmbedding] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* deleteSQL = "DELETE FROM embedding_index WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::deleteEmbedding] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE && sqlite3_changes(db) > 0);

    if (!success) {
        std::cerr << "[EmbeddingIndexManager::deleteEmbedding] 删除失败: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}

bool EmbeddingIndexManager::deleteEmbeddingByMemory(int memoryId, const std::string& memoryType, const std::string& dbPath) {
    if (!ensureEmbeddingTableExists(dbPath)) {
        return false;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::deleteEmbeddingByMemory] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* deleteSQL = "DELETE FROM embedding_index WHERE memory_id = ? AND memory_type = ?";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::deleteEmbeddingByMemory] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_int(stmt, 1, memoryId);
    sqlite3_bind_text(stmt, 2, memoryType.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE);
    int deleted_count = sqlite3_changes(db);

    if (success && deleted_count > 0) {
        std::cout << "[EmbeddingIndexManager] ✓ 级联删除 embedding: memory_id=" << memoryId 
                  << ", memory_type=" << memoryType << ", 删除" << deleted_count << "条" << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return success;
}

std::vector<EmbeddingRecord> EmbeddingIndexManager::searchEmbeddings(const std::vector<float>& queryEmbedding, 
                                                                   int topK, const std::string& dbPath) {
    std::vector<EmbeddingRecord> results;
    
    if (queryEmbedding.empty()) {
        std::cerr << "[EmbeddingIndexManager::searchEmbeddings] 查询embedding为空" << std::endl;
        return results;
    }
    
    if (!ensureEmbeddingTableExists(dbPath)) {
        return results;
    }
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::searchEmbeddings] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return results;
    }
    
    // 查询所有 embedding 记录
    const char* selectSQL = "SELECT id, memory_id, memory_type, embedding, metadata, created_at FROM embedding_index";
    sqlite3_stmt* stmt = nullptr;
    
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::searchEmbeddings] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return results;
    }
    
    // 存储所有 embedding 和其相似度
    std::vector<std::pair<EmbeddingRecord, float>> candidatesWithSimilarity;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EmbeddingRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.memory_id = sqlite3_column_int(stmt, 1);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        // 反序列化 embedding
        const void* blobData = sqlite3_column_blob(stmt, 3);
        int blobSize = sqlite3_column_bytes(stmt, 3);
        if (blobData && blobSize > 0) {
            std::string blobStr(static_cast<const char*>(blobData), blobSize);
            record.embedding = VectorSerializer::deserialize(blobStr);
        }
        
        const char* metaText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (metaText) {
            record.metadata = metaText;
        }
        
        const char* createdText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (createdText) {
            record.created_at = createdText;
        }
        
        // 计算相似度
        if (!record.embedding.empty()) {
            float similarity = cosineSimilarity(queryEmbedding, record.embedding);
            candidatesWithSimilarity.push_back({record, similarity});
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    // 按相似度降序排序
    std::sort(candidatesWithSimilarity.begin(), candidatesWithSimilarity.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // 取前 topK 个
    int count = 0;
    for (const auto& pair : candidatesWithSimilarity) {
        if (count >= topK) break;
        results.push_back(pair.first);
        count++;
    }
    
    std::cout << "[EmbeddingIndexManager::searchEmbeddings] 搜索完成, 返回 " << results.size() << " 条结果" << std::endl;
    
    return results;
}

EmbeddingRecord EmbeddingIndexManager::getEmbeddingById(int id, const std::string& dbPath) {
    EmbeddingRecord record;

    if (!ensureEmbeddingTableExists(dbPath)) {
        return record;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::getEmbeddingById] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return record;
    }

    const char* selectSQL = "SELECT id, memory_id, memory_type, embedding, metadata, created_at FROM embedding_index WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[EmbeddingIndexManager::getEmbeddingById] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return record;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.memory_id = sqlite3_column_int(stmt, 1);
        const char* typeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (typeText) {
            record.memory_type = typeText;
        }

        const void* blobData = sqlite3_column_blob(stmt, 3);
        int blobSize = sqlite3_column_bytes(stmt, 3);
        if (blobData && blobSize > 0) {
            std::string blobStr(static_cast<const char*>(blobData), blobSize);
            record.embedding = VectorSerializer::deserialize(blobStr);
        }

        const char* metaText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (metaText) {
            record.metadata = metaText;
        }

        const char* createdText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (createdText) {
            record.created_at = createdText;
        }

        std::cout << "[EmbeddingIndexManager::getEmbeddingById] ✓ 找到 embedding, id=" << id
                  << ", memory_id=" << record.memory_id << ", type=" << record.memory_type << std::endl;
    } else {
        std::cout << "[EmbeddingIndexManager::getEmbeddingById] 未找到 id=" << id << " 对应的 embedding" << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return record;
}

// 索引一致性检查
EmbeddingIndexManager::ConsistencyReport EmbeddingIndexManager::checkIndexConsistency(int userId, const std::string& dbPath) {
    ConsistencyReport report;
    
    std::cout << "\n[IndexConsistency] ========== 开始索引一致性检查 ==========" << std::endl;
    std::cout << "[IndexConsistency] User ID  : " << userId << std::endl;
    std::cout << "[IndexConsistency] Database : " << dbPath << std::endl;
    
    if (!ensureEmbeddingTableExists(dbPath)) {
        std::cerr << "[IndexConsistency] 数据库表不存在" << std::endl;
        return report;
    }
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[IndexConsistency] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return report;
    }
    
    std::cout << "\n[IndexConsistency] -- 步骤 1: 扫描 embedding 索引" << std::endl;
    
    // 查询所有 embedding 记录
    const char* selectSQL = "SELECT id, memory_id, memory_type, metadata FROM embedding_index";
    sqlite3_stmt* stmt = nullptr;
    
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[IndexConsistency] 准备SQL失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return report;
    }
    
    struct EmbeddingInfo {
        int id;
        int memory_id;
        std::string memory_type;
        std::string metadata;
    };
    
    std::vector<EmbeddingInfo> allEmbeddings;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EmbeddingInfo info;
        info.id = sqlite3_column_int(stmt, 0);
        info.memory_id = sqlite3_column_int(stmt, 1);
        const char* typeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (typeText) info.memory_type = typeText;
        const char* metaText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (metaText) info.metadata = metaText;
        
        allEmbeddings.push_back(info);
    }
    
    sqlite3_finalize(stmt);
    
    report.total_embeddings = allEmbeddings.size();
    std::cout << "[IndexConsistency]   总计: " << report.total_embeddings << " 条 embedding" << std::endl;
    
    if (allEmbeddings.empty()) {
        std::cout << "\n[IndexConsistency] ========== 检查完成 (索引为空) ==========\n" << std::endl;
        sqlite3_close(db);
        return report;
    }
    
    std::cout << "\n[IndexConsistency] -- 步骤 2: 验证记忆记录是否存在" << std::endl;
    
    // 根据 memory_type 验证记忆记录
    for (const auto& emb : allEmbeddings) {
        bool valid = false;
        int record_user_id = -1;
        
        // 根据类型查询对应的记忆表
        if (emb.memory_type == "episodic") {
            const char* checkSQL = "SELECT user_id FROM episodic_memories WHERE id = ?";
            sqlite3_stmt* check_stmt = nullptr;
            if (sqlite3_prepare_v2(db, checkSQL, -1, &check_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(check_stmt, 1, emb.memory_id);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    record_user_id = sqlite3_column_int(check_stmt, 0);
                    valid = true;
                }
                sqlite3_finalize(check_stmt);
            }
        }
        else if (emb.memory_type == "semantic") {
            const char* checkSQL = "SELECT user_id FROM semantic_memories WHERE id = ?";
            sqlite3_stmt* check_stmt = nullptr;
            if (sqlite3_prepare_v2(db, checkSQL, -1, &check_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(check_stmt, 1, emb.memory_id);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    record_user_id = sqlite3_column_int(check_stmt, 0);
                    valid = true;
                }
                sqlite3_finalize(check_stmt);
            }
        }
        else if (emb.memory_type == "core") {
            const char* checkSQL = "SELECT user_id FROM core_memory_blocks WHERE id = ?";
            sqlite3_stmt* check_stmt = nullptr;
            if (sqlite3_prepare_v2(db, checkSQL, -1, &check_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(check_stmt, 1, emb.memory_id);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    record_user_id = sqlite3_column_int(check_stmt, 0);
                    valid = true;
                }
                sqlite3_finalize(check_stmt);
            }
        }
        else if (emb.memory_type == "procedural") {
            const char* checkSQL = "SELECT user_id FROM procedural_memories WHERE id = ?";
            sqlite3_stmt* check_stmt = nullptr;
            if (sqlite3_prepare_v2(db, checkSQL, -1, &check_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(check_stmt, 1, emb.memory_id);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    record_user_id = sqlite3_column_int(check_stmt, 0);
                    valid = true;
                }
                sqlite3_finalize(check_stmt);
            }
        }
        else if (emb.memory_type == "screenshot" || emb.memory_type == "image" || emb.memory_type == "audio") {
            // 从 metadata 中解析 user_id（格式如: {"user_id":1}）
            try {
                size_t pos = emb.metadata.find("\"user_id\":");
                if (pos != std::string::npos) {
                    pos += 10; // 跳过 "user_id":
                    while (pos < emb.metadata.length() && std::isspace(emb.metadata[pos])) pos++;
                    size_t end_pos = pos;
                    while (end_pos < emb.metadata.length() && std::isdigit(emb.metadata[end_pos])) end_pos++;
                    if (end_pos > pos) {
                        record_user_id = std::stoi(emb.metadata.substr(pos, end_pos - pos));
                    }
                }
            } catch (...) {
                // 解析失败，保持 record_user_id = -1
            }
            
            const char* checkSQL = "SELECT id FROM multimodal_memories WHERE id = ?";
            sqlite3_stmt* check_stmt = nullptr;
            if (sqlite3_prepare_v2(db, checkSQL, -1, &check_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(check_stmt, 1, emb.memory_id);
                if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                    valid = true;
                }
                sqlite3_finalize(check_stmt);
            }
        }
        
        // 分类统计
        if (!valid) {
            report.orphaned_embeddings++;
            report.orphaned_ids.push_back(emb.id);
        } else if (record_user_id != -1 && record_user_id != userId) {
            report.user_mismatch++;
        } else {
            report.valid_embeddings++;
        }
    }
    
    sqlite3_close(db);
    
    std::cout << "[IndexConsistency]   孤儿 embedding  : " << report.orphaned_embeddings << " 条" << std::endl;
    std::cout << "[IndexConsistency]   user_id 不匹配: " << report.user_mismatch << " 条" << std::endl;
    std::cout << "[IndexConsistency]   有效 embedding  : " << report.valid_embeddings << " 条" << std::endl;
    
    if (report.orphaned_embeddings > 0) {
        std::cout << "\n[IndexConsistency] 孤儿 embedding ID 列表 (前 20 个):" << std::endl;
        int count = 0;
        for (int id : report.orphaned_ids) {
            if (count >= 20) {
                std::cout << "[IndexConsistency]   ... (共 " << report.orphaned_ids.size() << " 个)" << std::endl;
                break;
            }
            std::cout << "[IndexConsistency]   - ID: " << id << std::endl;
            count++;
        }
    }
    
    std::cout << "\n[IndexConsistency] ========== 检查完成 ==========\n" << std::endl;
    
    return report;
}

bool EmbeddingIndexManager::cleanOrphanedEmbeddings(const std::vector<int>& embeddingIds, const std::string& dbPath) {
    if (embeddingIds.empty()) {
        std::cout << "[IndexConsistency] 没有需要清理的 embedding" << std::endl;
        return true;
    }
    
    std::cout << "\n[IndexConsistency] ========== 开始清理孤儿 embedding ==========" << std::endl;
    std::cout << "[IndexConsistency] 待清理数量: " << embeddingIds.size() << " 条" << std::endl;
    
    if (!ensureEmbeddingTableExists(dbPath)) {
        return false;
    }
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[IndexConsistency] 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // 开启事务
    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    int deleted_count = 0;
    const char* deleteSQL = "DELETE FROM embedding_index WHERE id = ?";
    
    for (int id : embeddingIds) {
        sqlite3_stmt* stmt = nullptr;
        rc = sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, id);
            if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0) {
                deleted_count++;
            }
            sqlite3_finalize(stmt);
        }
    }
    
    // 提交事务
    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_close(db);
    
    std::cout << "[IndexConsistency] 已成功清理: " << deleted_count << " 条 embedding" << std::endl;
    std::cout << "[IndexConsistency] ========== 清理完成 ==========\n" << std::endl;
    
    return true;
}

// 添加FAISS相关方法实现
bool EmbeddingIndexManager::initializeFAISSIndex(int dimension, const std::string& indexPath) {
    // TODO: 实现初始化FAISS索引的逻辑
    std::cout << "Initializing FAISS index with dimension: " << dimension << std::endl;
    return true;
}

bool EmbeddingIndexManager::saveFAISSIndex(const std::string& indexPath) {
    // TODO: 实现保存FAISS索引的逻辑
    std::cout << "Saving FAISS index to: " << indexPath << std::endl;
    return true;
}

bool EmbeddingIndexManager::loadFAISSIndex(const std::string& indexPath) {
    // TODO: 实现加载FAISS索引的逻辑
    std::cout << "Loading FAISS index from: " << indexPath << std::endl;
    return true;
}

std::vector<std::pair<int, float>> EmbeddingIndexManager::searchEmbeddingsFAISS(const std::vector<float>& queryEmbedding, int topK) {
    // TODO: 实现使用FAISS搜索嵌入向量的逻辑
    std::cout << "Searching embeddings with FAISS, topK: " << topK << std::endl;
    return std::vector<std::pair<int, float>>();
}
