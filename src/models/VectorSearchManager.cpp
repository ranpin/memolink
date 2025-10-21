#include "VectorSearchManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

VectorSearchManager::VectorSearchManager(StorageManager& storage) 
    : storage_(storage), 
      recallMemoryManager_(storage),
      resourceMemoryManager_(storage),
      textEmbeddingGenerator_() {
}

std::vector<float> VectorSearchManager::generateTextEmbedding(const std::string& text) {
    // 使用TextEmbeddingGenerator生成文本embedding
    return textEmbeddingGenerator_.generateEmbedding(text);
}

float VectorSearchManager::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    if (normA == 0.0f || normB == 0.0f) {
        return 0.0f;
    }
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// 辅助函数：解析ISO8601格式的时间字符串
std::time_t parse_iso8601(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return std::time(nullptr); // 解析失败则视为当前时间
    }
    return std::mktime(&tm);
}

float VectorSearchManager::calculateTimeScore(const std::string& timestamp) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    
    auto memory_time_t = parse_iso8601(timestamp);
    
    double seconds_diff = std::difftime(now_time_t, memory_time_t);
    if (seconds_diff < 0) seconds_diff = 0; // 防止未来时间
    
    // 指数衰减：半衰期设为7天（604800秒）
    const double half_life = 604800.0;
    float time_score = std::exp(-std::log(2.0) * seconds_diff / half_life);
    
    return time_score;
}

void VectorSearchManager::normalizeScores(std::vector<HybridSearchResult>& results) {
    if (results.empty()) return;
    
    // 找到最大和最小得分
    float max_relevance = results[0].relevance_score;
    float min_relevance = results[0].relevance_score;
    float max_time = results[0].time_score;
    float min_time = results[0].time_score;
    
    for (const auto& result : results) {
        max_relevance = std::max(max_relevance, result.relevance_score);
        min_relevance = std::min(min_relevance, result.relevance_score);
        max_time = std::max(max_time, result.time_score);
        min_time = std::min(min_time, result.time_score);
    }
    
    // 归一化得分
    for (auto& result : results) {
        if (max_relevance != min_relevance) {
            result.relevance_score = (result.relevance_score - min_relevance) / (max_relevance - min_relevance);
        } else {
            result.relevance_score = 1.0f;
        }
        
        if (max_time != min_time) {
            result.time_score = (result.time_score - min_time) / (max_time - min_time);
        } else {
            result.time_score = 1.0f;
        }
    }
}

std::vector<HybridSearchResult> VectorSearchManager::vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit) {
    std::vector<HybridSearchResult> results;
    
    // 使用RecallMemoryManager获取所有用户记忆
    auto allMemories = recallMemoryManager_.list_recall_memories(userId, 0);
    
    // 计算每个记忆的相似度
    for (const auto& memory : allMemories) {
        // 获取记忆的embedding
        auto memoryEmbedding = recallMemoryManager_.getEmbedding(memory.id);
        
        if (!memoryEmbedding.empty()) {
            // 计算相似度
            float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
            
            HybridSearchResult result;
            result.id = memory.id;
            result.type = "text";
            result.content = memory.content;
            result.timestamp = memory.created_at;
            result.relevance_score = similarity;
            result.time_score = calculateTimeScore(memory.created_at);
            result.hybrid_score = similarity; // 纯向量搜索只考虑相似度
            
            results.push_back(result);
        }
    }
    
    // 按相似度排序
    std::sort(results.begin(), results.end(), 
              [](const HybridSearchResult& a, const HybridSearchResult& b) {
                  return a.relevance_score > b.relevance_score;
              });
    
    // 限制结果数量
    if (results.size() > static_cast<size_t>(limit)) {
        results.resize(limit);
    }
    
    return results;
}

std::vector<HybridSearchResult> VectorSearchManager::hybridSearch(int userId, const std::string& query, int limit) {
    // 先进行全文搜索
    auto textResults = recallMemoryManager_.search_recall_memories(userId, query);
    
    // 然后进行向量搜索
    auto queryEmbedding = generateTextEmbedding(query);
    auto vectorResults = vectorSearch(userId, queryEmbedding, limit);
    
    // 合并结果并去重
    std::vector<HybridSearchResult> hybridResults;
    
    // 添加文本搜索结果
    for (const auto& result : textResults) {
        HybridSearchResult hybridResult;
        hybridResult.id = result.id;
        hybridResult.type = "text";
        hybridResult.content = result.content;
        hybridResult.timestamp = result.created_at;
        hybridResult.relevance_score = 1.0f; // 文本匹配得分为1
        hybridResult.time_score = calculateTimeScore(result.created_at);
        hybridResult.hybrid_score = 1.0f; // 混合得分为1
        
        hybridResults.push_back(hybridResult);
    }
    
    // 添加向量搜索结果（避免重复）
    for (const auto& vectorResult : vectorResults) {
        bool found = false;
        for (const auto& existing : hybridResults) {
            if (existing.id == vectorResult.id) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            hybridResults.push_back(vectorResult);
        }
    }
    
    // 按时间排序（简化实现）
    std::sort(hybridResults.begin(), hybridResults.end(),
              [](const HybridSearchResult& a, 
                 const HybridSearchResult& b) {
                  return a.timestamp > b.timestamp; // 最新的在前
              });
    
    // 取前limit个结果
    if (hybridResults.size() > static_cast<size_t>(limit)) {
        hybridResults.resize(limit);
    }
    
    return hybridResults;
}

std::vector<HybridSearchResult> VectorSearchManager::multimodalSearch(int userId, const std::string& query, int limit) {
    // 生成查询文本的embedding
    auto queryEmbedding = generateTextEmbedding(query);
    
    // 搜索资源记忆（图像、音频等）
    auto resourceResults = resourceMemoryManager_.searchByEmbedding(userId, queryEmbedding, limit);
    
    // 转换为混合搜索结果
    std::vector<HybridSearchResult> results;
    for (const auto& resource : resourceResults) {
        HybridSearchResult result;
        result.id = resource.id;
        result.type = resource.resource_type;
        result.content = resource.content;
        result.timestamp = resource.created_at;
        result.relevance_score = 1.0f; // 简化处理
        result.time_score = calculateTimeScore(resource.created_at);
        result.hybrid_score = 1.0f;
        
        results.push_back(result);
    }
    
    return results;
}