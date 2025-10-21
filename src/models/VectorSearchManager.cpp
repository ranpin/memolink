#include "VectorSearchManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// 计算两个向量之间的余弦相似度
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

// 生成文本embedding（占位符实现）
std::vector<float> VectorSearchManager::generateTextEmbedding(const std::string& text) {
    // 简化的实现，实际应该调用模型API
    std::vector<float> embedding(128, 0.1f);
    return embedding;
}

// 计算时间得分
float VectorSearchManager::calculateTimeScore(const std::string& createdAt) {
    // 简化的实现，实际应该根据时间差计算得分
    return 1.0f;
}

// 基于向量的语义搜索
std::vector<RecallMemoryManager::RecallMemoryItem> VectorSearchManager::vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit) {
    std::vector<RecallMemoryManager::RecallMemoryItem> results;
    
    // 获取所有回忆记忆
    auto allMemories = recallMemoryManager_.list_recall_memories(userId, 0);
    
    // 计算每个记忆与查询的相似度
    std::vector<std::pair<RecallMemoryManager::RecallMemoryItem, float>> scoredMemories;
    
    for (const auto& memory : allMemories) {
        // 获取记忆的embedding
        auto memoryEmbedding = recallMemoryManager_.getEmbedding(memory.id);
        
        if (!memoryEmbedding.empty()) {
            // 计算余弦相似度
            float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
            
            scoredMemories.push_back({memory, similarity});
        }
    }
    
    // 按相似度排序
    std::sort(scoredMemories.begin(), scoredMemories.end(),
              [](const std::pair<RecallMemoryManager::RecallMemoryItem, float>& a, 
                 const std::pair<RecallMemoryManager::RecallMemoryItem, float>& b) {
                  return a.second > b.second;
              });
    
    // 取前limit个结果
    int count = 0;
    for (const auto& pair : scoredMemories) {
        if (count >= limit) break;
        results.push_back(pair.first);
        count++;
    }
    
    return results;
}

// 混合搜索（文本相关性 + 时间相关性）
std::vector<RecallMemoryManager::RecallMemoryItem> VectorSearchManager::hybridSearch(int userId, const std::string& query, int limit) {
    // 先进行全文搜索
    auto textResults = recallMemoryManager_.search_recall_memories(userId, query);
    
    // 然后进行向量搜索
    auto queryEmbedding = generateTextEmbedding(query);
    auto vectorResults = vectorSearch(userId, queryEmbedding, limit);
    
    // 合并结果并去重
    std::vector<RecallMemoryManager::RecallMemoryItem> hybridResults;
    
    // 添加文本搜索结果
    for (const auto& result : textResults) {
        hybridResults.push_back(result);
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
              [](const RecallMemoryManager::RecallMemoryItem& a, 
                 const RecallMemoryManager::RecallMemoryItem& b) {
                  return a.created_at > b.created_at; // 最新的在前
              });
    
    // 取前limit个结果
    if (hybridResults.size() > static_cast<size_t>(limit)) {
        hybridResults.resize(limit);
    }
    
    return hybridResults;
}

// 多模态数据搜索
std::vector<ResourceMemoryManager::ResourceMemoryItem> VectorSearchManager::multimodalSearch(int userId, const std::vector<float>& queryEmbedding, int limit) {
    // 搜索资源记忆（图像、音频等）
    return resourceMemoryManager_.searchByEmbedding(userId, queryEmbedding, limit);
}