#ifndef VECTOR_SEARCH_MANAGER_H
#define VECTOR_SEARCH_MANAGER_H
#include <algorithm>

#include "legacy/core-v1/RecallMemoryManager.h"
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "infra/embedding/TextEmbeddingGenerator.h"
#include "infra/storage/StorageManager.h"
#include <vector>
#include <string>
#include <memory>

// 前置声明
// CLIP 处理器依赖 OpenCV 且通过 popen 调 Python，只在桌面多模态构建（MEMOLINK_ENABLE_CAPTURE）
// 里编入；端侧文本记忆构建用 MEMOLINK_HAS_CLIP 宏整体隔离掉。
#ifdef MEMOLINK_HAS_CLIP
class CLIPProcessor;
#endif

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
#ifdef MEMOLINK_HAS_CLIP
    std::unique_ptr<CLIPProcessor> clipProcessor_;
#endif

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