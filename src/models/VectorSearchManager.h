#ifndef VECTOR_SEARCH_MANAGER_H
#define VECTOR_SEARCH_MANAGER_H

#include "../database/managers/RecallMemoryManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "TextEmbeddingGenerator.h"
#include <vector>
#include <string>
#include <memory>

// 混合检索结果结构
struct HybridSearchResult {
    int id;
    std::string type;  // "text" or "multimodal"
    std::string content;
    std::string timestamp;
    float relevance_score;
    float time_score;
    float hybrid_score;
};

class VectorSearchManager {
public:
    VectorSearchManager(StorageManager& storage);
    
    // 混合检索：结合时间相关性和语义相似性
    std::vector<HybridSearchResult> hybridSearch(int userId, const std::string& query, int limit = 10);
    
    // 纯向量检索
    std::vector<HybridSearchResult> vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit = 10);
    
    // 多模态联合检索
    std::vector<HybridSearchResult> multimodalSearch(int userId, const std::string& query, int limit = 10);
    
    // 通过文本生成embedding
    std::vector<float> generateTextEmbedding(const std::string& text);
    
private:
    StorageManager& storage_;
    RecallMemoryManager recallMemoryManager_;
    ResourceMemoryManager resourceMemoryManager_;
    TextEmbeddingGenerator textEmbeddingGenerator_;
    
    // 计算余弦相似度
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    
    // 计算时间得分（越近的记录得分越高）
    float calculateTimeScore(const std::string& timestamp);
    
    // 归一化得分
    void normalizeScores(std::vector<HybridSearchResult>& results);
};

#endif // VECTOR_SEARCH_MANAGER_H