#include "VectorSearchManager.h"
#include "CLIPProcessor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

VectorSearchManager::VectorSearchManager(StorageManager& storage) 
    : storage_(storage), 
      recallMemoryManager_(storage),
      resourceMemoryManager_(storage),
      textEmbeddingGenerator_() {
    std::cout << "[VectorSearchManager] 初始化..." << std::endl;
    
    // 初始化CLIP处理器
    std::cout << "[VectorSearchManager] 初始化CLIP处理器..." << std::endl;
    clipProcessor_ = std::make_unique<CLIPProcessor>();
    std::cout << "[VectorSearchManager] CLIP处理器初始化完成" << std::endl;
    
    std::cout << "[VectorSearchManager] 初始化完成" << std::endl;
}

std::vector<float> VectorSearchManager::generateTextEmbedding(const std::string& text) {
    std::cout << "[VectorSearchManager] 开始生成查询文本embedding..." << std::endl;
    std::cout << "[VectorSearchManager] 查询文本: \"" << text << "\"" << std::endl;
    
    // 优先使用TextEmbeddingGenerator生成文本embedding
    std::cout << "[VectorSearchManager] 使用TextEmbeddingGenerator生成查询文本embedding..." << std::endl;
    std::vector<float> embedding = textEmbeddingGenerator_.generateEmbedding(text);
    if (!embedding.empty()) {
        std::cout << "[VectorSearchManager] TextEmbeddingGenerator生成查询文本embedding完成，维度: " << embedding.size() << std::endl;
        return embedding;
    } else {
        std::cout << "[VectorSearchManager] TextEmbeddingGenerator返回空embedding，尝试其他方法..." << std::endl;
    }
    
    // 备选用CLIP处理器生成文本embedding
    if (clipProcessor_) {
        std::cout << "[VectorSearchManager] 尝试使用CLIP处理器生成查询文本embedding..." << std::endl;
        try {
            std::vector<float> clipEmbedding = clipProcessor_->encodeText(text);
            if (!clipEmbedding.empty()) {
                std::cout << "[VectorSearchManager] CLIP处理器生成查询文本embedding成功，维度: " << clipEmbedding.size() << std::endl;
                return clipEmbedding;
            } else {
                std::cout << "[VectorSearchManager] CLIP处理器返回空embedding" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[VectorSearchManager] CLIP文本编码失败: " << e.what() << std::endl;
        }
    }
    
    // 如果所有方法都失败，返回空向量
    std::cout << "[VectorSearchManager] 所有方法都失败，返回空embedding" << std::endl;
    return {};
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


float VectorSearchManager::calculateTimeScore(const std::string& timestamp) {
    // 简单的时间衰减函数：越近的时间得分越高
    // 这里可以实现更复杂的时间衰减算法
    return 1.0f;
}

void VectorSearchManager::normalizeScores(std::vector<HybridSearchResult>& results) {
    if (results.empty()) return;

    // 找到最大相关性得分和时间得分
    float maxRelevance = 0.0f;
    float maxTime = 0.0f;
    for (const auto& result : results) {
        maxRelevance = std::max(maxRelevance, result.relevance_score);
        maxTime = std::max(maxTime, result.time_score);
    }

    // 归一化得分
    for (auto& result : results) {
        if (maxRelevance > 0.0f) {
            result.relevance_score /= maxRelevance;
        }
        if (maxTime > 0.0f) {
            result.time_score /= maxTime;
        }
        // 计算混合得分 (这里简单地取平均值，可以调整权重)
        result.hybrid_score = (result.relevance_score + result.time_score) / 2.0f;
    }
}

std::vector<HybridSearchResult> VectorSearchManager::vectorSearch(int userId, const std::vector<float>& queryEmbedding, int limit) {
    std::cout << "\n********************************************************************************" << std::endl;
    std::cout << "[VectorSearchManager] 开始向量搜索..." << std::endl;
    std::cout << "[VectorSearchManager] 用户ID: " << userId << std::endl;
    std::cout << "[VectorSearchManager] 查询向量维度: " << queryEmbedding.size() << std::endl;
    std::cout << "[VectorSearchManager] 限制结果数量: " << limit << std::endl;
    std::cout << "********************************************************************************" << std::endl;
    
    std::vector<HybridSearchResult> results;

    // 搜索回忆记忆
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 搜索回忆记忆..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    auto recallMemories = recallMemoryManager_.list_recall_memories(userId, limit > 0 ? limit * 2 : 0);
    std::cout << "[VectorSearchManager] 找到 " << recallMemories.size() << " 个回忆记忆" << std::endl;
    
    int processedRecall = 0;
    for (const auto& memory : recallMemories) {
        auto memoryEmbedding = recallMemoryManager_.getEmbedding(memory.id);
        if (!memoryEmbedding.empty() && memoryEmbedding.size() == queryEmbedding.size()) {
            float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
            
            // 输出相似度分数用于调试
            std::cout << "[VectorSearchManager] 回忆记忆 ID " << memory.id << " 相似度: " << similarity << std::endl;
            
            // 只有相似度大于阈值的结果才被考虑
            if (similarity > 0.1) {  // 设置一个较低的阈值
                HybridSearchResult result;
                result.id = memory.id;
                result.type = "recall";
                result.content = memory.content;
                result.timestamp = memory.created_at;
                result.relevance_score = similarity;
                result.time_score = calculateTimeScore(memory.created_at);
                
                results.push_back(result);
                processedRecall++;
            }
        }
    }
    std::cout << "[VectorSearchManager] 处理了 " << processedRecall << " 个回忆记忆" << std::endl;

    // 搜索资源记忆
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 搜索资源记忆..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    auto resourceMemories = resourceMemoryManager_.list_resource_memories(userId, limit > 0 ? limit * 2 : 0);
    std::cout << "[VectorSearchManager] 找到 " << resourceMemories.size() << " 个资源记忆" << std::endl;
    
    int processedResource = 0;
    for (const auto& memory : resourceMemories) {
        auto memoryEmbedding = resourceMemoryManager_.getEmbedding(memory.id);
        if (!memoryEmbedding.empty() && memoryEmbedding.size() == queryEmbedding.size()) {
            float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
            
            // 输出相似度分数用于调试
            std::cout << "[VectorSearchManager] 资源记忆 ID " << memory.id << " 相似度: " << similarity << std::endl;
            
            // 只有相似度大于阈值的结果才被考虑
            if (similarity > 0.1) {  // 设置一个较低的阈值
                HybridSearchResult result;
                result.id = memory.id;
                result.type = "resource";
                result.content = memory.description;  // 将content改为description
                result.timestamp = memory.created_at;
                result.relevance_score = similarity;
                result.time_score = calculateTimeScore(memory.created_at);
                
                results.push_back(result);
                processedResource++;
            }
        }
    }
    std::cout << "[VectorSearchManager] 处理了 " << processedResource << " 个资源记忆" << std::endl;

    // 如果没有找到任何结果，尝试降低阈值
    if (results.empty()) {
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[VectorSearchManager] 未找到匹配结果，尝试降低阈值..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        
        // 重新处理回忆记忆，使用更低的阈值
        for (const auto& memory : recallMemories) {
            auto memoryEmbedding = recallMemoryManager_.getEmbedding(memory.id);
            if (!memoryEmbedding.empty() && memoryEmbedding.size() == queryEmbedding.size()) {
                float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
                
                // 输出相似度分数用于调试
                std::cout << "[VectorSearchManager] 回忆记忆 ID " << memory.id << " 相似度(降级): " << similarity << std::endl;
                
                // 使用更低的阈值
                if (similarity > 0.01) {
                    HybridSearchResult result;
                    result.id = memory.id;
                    result.type = "recall";
                    result.content = memory.content;
                    result.timestamp = memory.created_at;
                    result.relevance_score = similarity;
                    result.time_score = calculateTimeScore(memory.created_at);
                    
                    results.push_back(result);
                }
            }
        }
        
        // 重新处理资源记忆，使用更低的阈值
        for (const auto& memory : resourceMemories) {
            auto memoryEmbedding = resourceMemoryManager_.getEmbedding(memory.id);
            if (!memoryEmbedding.empty() && memoryEmbedding.size() == queryEmbedding.size()) {
                float similarity = cosineSimilarity(queryEmbedding, memoryEmbedding);
                
                // 输出相似度分数用于调试
                std::cout << "[VectorSearchManager] 资源记忆 ID " << memory.id << " 相似度(降级): " << similarity << std::endl;
                
                // 使用更低的阈值
                if (similarity > 0.01) {
                    HybridSearchResult result;
                    result.id = memory.id;
                    result.type = "resource";
                    result.content = memory.description;
                    result.timestamp = memory.created_at;
                    result.relevance_score = similarity;
                    result.time_score = calculateTimeScore(memory.created_at);
                    
                    results.push_back(result);
                }
            }
        }
        
        std::cout << "[VectorSearchManager] 降级搜索后找到 " << results.size() << " 个结果" << std::endl;
    }

    // 归一化得分
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 归一化得分..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    normalizeScores(results);

    // 按混合得分排序
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 排序结果..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::sort(results.begin(), results.end(),
              [](const HybridSearchResult& a, const HybridSearchResult& b) {
                  return a.hybrid_score > b.hybrid_score;
              });

    // 限制结果数量
    if (limit > 0 && results.size() > static_cast<size_t>(limit)) {
        std::cout << "[VectorSearchManager] 限制结果数量从 " << results.size() << " 到 " << limit << std::endl;
        results.resize(limit);
    }
    
    std::cout << "\n********************************************************************************" << std::endl;
    std::cout << "[VectorSearchManager] 向量搜索完成，返回 " << results.size() << " 个结果" << std::endl;
    std::cout << "********************************************************************************\n" << std::endl;
    return results;
}

std::vector<HybridSearchResult> VectorSearchManager::hybridSearch(int userId, const std::string& query, int limit) {
    std::cout << "\n================================================================================" << std::endl;
    std::cout << "[VectorSearchManager] 开始混合搜索..." << std::endl;
    std::cout << "[VectorSearchManager] 查询: \"" << query << "\"" << std::endl;
    std::cout << "[VectorSearchManager] 用户ID: " << userId << std::endl;
    std::cout << "[VectorSearchManager] 限制结果数量: " << limit << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    // 生成查询文本的embedding
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 生成查询文本的embedding..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::vector<float> queryEmbedding = generateTextEmbedding(query);
    std::cout << "[VectorSearchManager] 查询embedding维度: " << queryEmbedding.size() << std::endl;
    
    // 检查embedding是否为空
    if (queryEmbedding.empty()) {
        std::cerr << "[VectorSearchManager] 查询embedding为空，返回空结果" << std::endl;
        return {};
    }
    
    // 输出查询embedding的部分内容用于调试
    std::cout << "[VectorSearchManager] 查询embedding前10维: ";
    for (int i = 0; i < 10 && i < queryEmbedding.size(); i++) {
        std::cout << queryEmbedding[i] << " ";
    }
    std::cout << std::endl;
    
    // 执行向量搜索
    std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
    std::cout << "[VectorSearchManager] 执行向量搜索..." << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    auto results = vectorSearch(userId, queryEmbedding, limit);
    
    // 如果向量搜索没有返回结果，尝试执行基本的文本搜索作为退化策略
    if (results.empty()) {
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[VectorSearchManager] 向量搜索未找到结果，尝试基本文本搜索..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        
        // 这里可以实现基于关键词匹配的基本文本搜索
        // 作为示例，我们简单地返回空结果
        std::cout << "[VectorSearchManager] 基本文本搜索未实现，返回空结果" << std::endl;
    }
    
    std::cout << "\n================================================================================" << std::endl;
    std::cout << "[VectorSearchManager] 混合搜索完成，返回 " << results.size() << " 个结果" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    // 输出搜索结果用于调试
    if (!results.empty()) {
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[VectorSearchManager] 搜索结果详情:" << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        for (const auto& result : results) {
            std::cout << "  ID: " << result.id 
                      << ", 类型: " << result.type 
                      << ", 相似度: " << result.relevance_score
                      << ", 内容预览: " << result.content.substr(0, 50) << "..." << std::endl;
        }
    } else {
        std::cout << "[VectorSearchManager] 未找到任何匹配结果" << std::endl;
    }
    
    return results;
}

std::vector<HybridSearchResult> VectorSearchManager::multimodalSearch(int userId, const std::string& query, int limit) {
    std::cout << "[VectorSearchManager] 开始多模态搜索..." << std::endl;
    std::cout << "[VectorSearchManager] 查询: \"" << query << "\"" << std::endl;
    
    // 目前多模态搜索与混合搜索实现相同
    // 后续可以扩展为支持图像等多模态输入
    auto results = hybridSearch(userId, query, limit);
    
    std::cout << "[VectorSearchManager] 多模态搜索完成" << std::endl;
    return results;
}