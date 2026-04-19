#ifndef EMBEDDING_INDEX_MANAGER_H
#define EMBEDDING_INDEX_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "../database/core/StorageManager.h"

struct EmbeddingRecord {
    int id = 0;
    int memory_id = 0;
    std::string memory_type;
    std::vector<float> embedding;
    std::string metadata;
    std::string created_at;
};

class EmbeddingIndexManager {
public:
    EmbeddingIndexManager() = default;
    ~EmbeddingIndexManager() = default;

    bool addEmbedding(int memoryId, const std::string& memoryType, 
                     const std::vector<float>& embedding, const std::string& metadata,
                     const std::string& dbPath = "../database/mirix.db");

    bool updateEmbedding(int id, const std::vector<float>& embedding, 
                        const std::string& metadata, const std::string& dbPath = "../database/mirix.db");

    bool deleteEmbedding(int id, const std::string& dbPath = "../database/mirix.db");
    
    // 级联删除：根据 memory_id 和 memory_type 删除 embedding
    bool deleteEmbeddingByMemory(int memoryId, const std::string& memoryType, 
                                const std::string& dbPath = "../database/mirix.db");

    std::vector<EmbeddingRecord> searchEmbeddings(const std::vector<float>& queryEmbedding, 
                                                 int topK = 10, const std::string& dbPath = "../database/mirix.db");

    EmbeddingRecord getEmbeddingById(int id, const std::string& dbPath = "../database/mirix.db");
    
    // 索引一致性检查
    struct ConsistencyReport {
        int total_embeddings = 0;
        int orphaned_embeddings = 0;  // 孤儿 embedding（记忆不存在）
        int user_mismatch = 0;         // user_id 不匹配
        int valid_embeddings = 0;      // 有效的 embedding
        std::vector<int> orphaned_ids;  // 孤儿 embedding 的 ID 列表
    };
    
    ConsistencyReport checkIndexConsistency(int userId, const std::string& dbPath = "../database/mirix.db");
    bool cleanOrphanedEmbeddings(const std::vector<int>& embeddingIds, const std::string& dbPath = "../database/mirix.db");
    
    // 为现有长期记忆补建 embedding 索引
    int rebuildEmbeddingsForExistingMemories(int userId, const std::string& dbPath = "../database/mirix.db");
    
    // 添加FAISS相关方法声明
    bool initializeFAISSIndex(int dimension, const std::string& indexPath = "../indexes/faiss_index.idx");
    bool saveFAISSIndex(const std::string& indexPath = "../indexes/faiss_index.idx");
    bool loadFAISSIndex(const std::string& indexPath = "../indexes/faiss_index.idx");

private:
    // 添加FAISS相关私有成员
    std::vector<std::pair<int, float>> searchEmbeddingsFAISS(const std::vector<float>& queryEmbedding, int topK = 10);
};

#endif // EMBEDDING_INDEX_MANAGER_H