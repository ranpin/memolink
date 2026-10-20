#ifndef EMBEDDING_INDEX_MANAGER_H
#define EMBEDDING_INDEX_MANAGER_H
#include <utility>

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

    std::vector<EmbeddingRecord> searchEmbeddings(const std::vector<float>& queryEmbedding, 
                                                 int topK = 10, const std::string& dbPath = "../database/mirix.db");

    EmbeddingRecord getEmbeddingById(int id, const std::string& dbPath = "../database/mirix.db");
    
    // 添加FAISS相关方法声明
    bool initializeFAISSIndex(int dimension, const std::string& indexPath = "../indexes/faiss_index.idx");
    bool saveFAISSIndex(const std::string& indexPath = "../indexes/faiss_index.idx");
    bool loadFAISSIndex(const std::string& indexPath = "../indexes/faiss_index.idx");

private:
    // 添加FAISS相关私有成员
    std::vector<std::pair<int, float>> searchEmbeddingsFAISS(const std::vector<float>& queryEmbedding, int topK = 10);
};

#endif // EMBEDDING_INDEX_MANAGER_H