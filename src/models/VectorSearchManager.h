#ifndef VECTOR_SEARCH_MANAGER_H
#define VECTOR_SEARCH_MANAGER_H

#include "../database/managers/MessageMemoryManager.h"
#include "../database/managers/MemoryOrchestrator.h"
#include "TextEmbeddingGenerator.h"
#include "EmbeddingIndexManager.h"
#include "../database/core/StorageManager.h"
#include <vector>
#include <string>
#include <memory>

// 前置声明
class CLIPProcessor;

// 检索配置结构
struct RetrievalConfig {
    // 混合得分权重
    float similarityWeight = 0.7f;  // 相似度权重
    float timeWeight = 0.3f;        // 时间衰减权重
    
    // 检索范围控制
    int expandFactor = 2;           // 从索引取 limit * expandFactor 条后重排
    float minHybridScore = 0.0f;    // 最小混合得分阈值（0表示不过滤）
    
    // 各类型记忆配额（用于分桶控制）
    int maxEpisodic = 4;
    int maxSemantic = 4;
    int maxCore = 3;
    int maxProcedural = 3;
    int maxMultimodal = 4;
};

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
    MessageMemoryManager messageMemoryManager_;
    TextEmbeddingGenerator textEmbeddingGenerator_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;
    RetrievalConfig config_;  // 检索配置

    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    float calculateTimeScore(const std::string& timestamp);
    void normalizeScores(std::vector<HybridSearchResult>& results);

public:
    VectorSearchManager(StorageManager& storage);
    VectorSearchManager(StorageManager& storage, const RetrievalConfig& config);
    
    // 配置访问接口
    const RetrievalConfig& getConfig() const { return config_; }
    void setConfig(const RetrievalConfig& config) { config_ = config; }
    
    std::vector<float> generateTextEmbedding(const std::string& text);
    std::vector<HybridSearchResult> vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit = 10);
    std::vector<HybridSearchResult> hybridSearch(int userId, const std::string& query, int limit = 10);
    std::vector<HybridSearchResult> multimodalSearch(int userId, const std::string& query, int limit = 10);
};

#endif // VECTOR_SEARCH_MANAGER_H