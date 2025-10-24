#ifndef VECTOR_SEARCH_MANAGER_H
#define VECTOR_SEARCH_MANAGER_H

#include "../database/managers/RecallMemoryManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "TextEmbeddingGenerator.h"
#include "../database/core/StorageManager.h"
#include <vector>
#include <string>
#include <memory>

// 前置声明
class CLIPProcessor;

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
private:
    StorageManager& storage_;
    RecallMemoryManager recallMemoryManager_;
    ResourceMemoryManager resourceMemoryManager_;
    TextEmbeddingGenerator textEmbeddingGenerator_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;

    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    float calculateTimeScore(const std::string& timestamp);
    void normalizeScores(std::vector<HybridSearchResult>& results);

public:
    VectorSearchManager(StorageManager& storage);
    std::vector<float> generateTextEmbedding(const std::string& text);
    std::vector<HybridSearchResult> vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit = 10);
    std::vector<HybridSearchResult> hybridSearch(int userId, const std::string& query, int limit = 10);
    std::vector<HybridSearchResult> multimodalSearch(int userId, const std::string& query, int limit = 10);
};

#endif // VECTOR_SEARCH_MANAGER_H