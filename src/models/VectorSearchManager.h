#ifndef VECTOR_SEARCH_MANAGER_H
#define VECTOR_SEARCH_MANAGER_H

#include "EmbeddingIndexManager.h"
#include "../database/managers/RecallMemoryManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../database/core/StorageManager.h"
#include <vector>
#include <string>

class VectorSearchManager {
private:
    StorageManager& storage_;
    EmbeddingIndexManager embeddingIndexManager_;
    RecallMemoryManager recallMemoryManager_;
    ResourceMemoryManager resourceMemoryManager_;

public:
    explicit VectorSearchManager(StorageManager& storage) 
        : storage_(storage), 
          embeddingIndexManager_(),
          recallMemoryManager_(storage),
          resourceMemoryManager_(storage) {}
    
    // 计算两个向量之间的余弦相似度
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    
    // 混合搜索（文本相关性 + 时间相关性）
    std::vector<RecallMemoryManager::RecallMemoryItem> hybridSearch(int userId, const std::string& query, int topK = 10);
    
    // 基于向量的语义搜索
    std::vector<RecallMemoryManager::RecallMemoryItem> vectorSearch(int userId, const std::vector<float>& queryEmbedding, int topK = 10);
    
    // 多模态数据搜索
    std::vector<ResourceMemoryManager::ResourceMemoryItem> multimodalSearch(int userId, const std::vector<float>& queryEmbedding, int topK = 10);
    
    // 生成文本embedding（占位符实现）
    std::vector<float> generateTextEmbedding(const std::string& text);
    
    // 计算时间得分
    float calculateTimeScore(const std::string& createdAt);
};

#endif // VECTOR_SEARCH_MANAGER_H