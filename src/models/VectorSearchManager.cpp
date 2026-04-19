#include "VectorSearchManager.h"
#include "CLIPProcessor.h"
#include "TextEmbeddingGenerator.h"
#include "../database/models/MemoryModels.h"
#include "../database/managers/EpisodicMemoryManager.h"
#include "../database/managers/SemanticMemoryManager.h"
#include "../database/managers/CoreMemoryBlockManager.h"
#include "../database/managers/ProceduralMemoryManager.h"
#include "../database/managers/SecretVaultManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>
#include <json/json.h>
#include <cctype>

VectorSearchManager::VectorSearchManager(StorageManager& storage) 
    : storage_(storage), 
      messageMemoryManager_(storage.getORMWrapper()),
      textEmbeddingGenerator_(&storage),
      config_() {  // 使用默认配置
    std::cout << "[VectorSearchManager] 初始化..." << std::endl;
    
    // 初始化CLIP处理器
    std::cout << "[VectorSearchManager] 初始化CLIP处理器..." << std::endl;
    clipProcessor_ = std::make_unique<CLIPProcessor>();
    std::cout << "[VectorSearchManager] CLIP处理器初始化完成" << std::endl;
    
    std::cout << "[VectorSearchManager] 初始化完成" << std::endl;
}

VectorSearchManager::VectorSearchManager(StorageManager& storage, const RetrievalConfig& config)
    : storage_(storage),
      messageMemoryManager_(storage.getORMWrapper()),
      textEmbeddingGenerator_(&storage),
      config_(config) {  // 使用自定义配置
    std::cout << "[VectorSearchManager] 初始化(自定义配置)..." << std::endl;
    
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
    std::vector<float> embedding = textEmbeddingGenerator_.generateTextEmbedding(text);
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
    // 实现真实的时间衰减策略：越近的时间得分越高
    // 解析 timestamp 并计算与当前时间的差值
    
    try {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        
        // 解析 timestamp 字符串（格式：YYYY-MM-DD HH:MM:SS）
        std::tm tm = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            // 如果解析失败，返回中等得分
            return 0.5f;
        }
        
        // 转换为 time_point
        auto memory_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        
        // 计算时间差（秒）
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - memory_time);
        double hours = duration.count() / 3600.0;
        
        // 时间衰减策略：
        // 1小时内：0.9-1.0
        // 1天内：0.6-0.9
        // 1周内：0.3-0.6
        // 1个月内：0.1-0.3
        // 更久：0.05
        
        if (hours < 1.0) {
            // 1小时内：线性衰减从 1.0 到 0.9
            return 1.0f - 0.1f * static_cast<float>(hours);
        } else if (hours < 24.0) {
            // 1天内：线性衰减从 0.9 到 0.6
            float t = static_cast<float>((hours - 1.0) / 23.0);  // 0~1
            return 0.9f - 0.3f * t;
        } else if (hours < 168.0) {  // 7天
            // 1周内：线性衰减从 0.6 到 0.3
            float t = static_cast<float>((hours - 24.0) / 144.0);  // 0~1
            return 0.6f - 0.3f * t;
        } else if (hours < 720.0) {  // 30天
            // 1个月内：线性衰减从 0.3 到 0.1
            float t = static_cast<float>((hours - 168.0) / 552.0);  // 0~1
            return 0.3f - 0.2f * t;
        } else {
            // 更久：固定低分
            return 0.05f;
        }
    } catch (const std::exception& e) {
        std::cerr << "[VectorSearchManager] 解析时间戳失败: " << e.what() << std::endl;
        return 0.5f;  // 失败时返回中等得分
    }
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
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "\n[VectorSearch] ========== 向量检索开始 ==========" << std::endl;
    std::cout << "[VectorSearch] User     : " << userId << std::endl;
    std::cout << "[VectorSearch] Dim      : " << queryEmbedding.size() << std::endl;
    std::cout << "[VectorSearch] Limit    : " << limit << std::endl;
    
    std::vector<HybridSearchResult> results;

    // Step 1: 向量索引检索
    std::cout << "\n[VectorSearch] -- Step 1: 向量索引检索" << std::endl;
    EmbeddingIndexManager embeddingIndex;
    std::string db_path = storage_.getDatabasePath();
    auto embeddingResults = embeddingIndex.searchEmbeddings(queryEmbedding, limit * config_.expandFactor, db_path);
    std::cout << "[VectorSearch]   索引返回: " << embeddingResults.size() << " 条" << std::endl;
    
    // Step 2: 映射为记忆记录
    std::cout << "\n[VectorSearch] -- Step 2: 映射为记忆记录" << std::endl;
    // 映射统计变量
    int total_embeddings = embeddingResults.size();
    int filtered_by_user = 0;
    int missing_records = 0;
    int mapped_ok = 0;

    // 预先创建所有类型的 Manager，避免在循环中重复创建
    auto& orm = storage_.getORMWrapper();
    EpisodicMemoryManager episodicMgr(orm);
    SemanticMemoryManager semanticMgr(orm);
    CoreMemoryBlockManager coreMgr(orm);
    ProceduralMemoryManager proceduralMgr(orm);
    SecretVaultManager secretMgr(orm);
    
    // 遍历 embedding 结果，转换为 HybridSearchResult
    for (const auto& embRec : embeddingResults) {
        // 从metadata中解析user_id, 仅保留当前用户的记录
        int record_user_id = -1;
        if (!embRec.metadata.empty()) {
            try {
                Json::Value meta;
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errs;
                if (reader->parse(embRec.metadata.c_str(), embRec.metadata.c_str() + embRec.metadata.size(), &meta, &errs)) {
                    if (meta.isMember("user_id")) {
                        record_user_id = meta["user_id"].asInt();
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[VectorSearchManager] 解析embedding metadata失败: " << e.what() << std::endl;
            }
        }
        if (record_user_id != -1 && record_user_id != userId) {
            filtered_by_user++;
            continue; // 过滤掉其他用户的 embedding
        }

        HybridSearchResult result;
        result.id = embRec.memory_id;
        result.type = embRec.memory_type;  // episodic, semantic, core, procedural, secret_vault, multimodal
        result.timestamp = embRec.created_at;
        
        // 根据 memory_type 从对应的 Manager 获取实际记忆内容
        std::string content;
        bool valid = false;
        try {
            if (embRec.memory_type == "episodic") {
                auto opt_memory = episodicMgr.getById(embRec.memory_id);
                if (opt_memory.has_value()) {
                    content = opt_memory.value().event_description;
                    valid = true;
                }
            } else if (embRec.memory_type == "semantic") {
                auto opt_memory = semanticMgr.getById(embRec.memory_id);
                if (opt_memory.has_value()) {
                    content = opt_memory.value().fact;
                    valid = true;
                }
            } else if (embRec.memory_type == "core") {
                auto opt_memory = coreMgr.getById(embRec.memory_id);
                if (opt_memory.has_value()) {
                    content = opt_memory.value().content;
                    valid = true;
                }
            } else if (embRec.memory_type == "procedural") {
                auto opt_memory = proceduralMgr.getById(embRec.memory_id);
                if (opt_memory.has_value()) {
                    content = opt_memory.value().description;
                    valid = true;
                }
            } else if (embRec.memory_type == "secret_vault") {
                auto opt_memory = secretMgr.getById(embRec.memory_id);
                if (opt_memory.has_value()) {
                    // 为避免泄露敏感内容，这里不返回真实内容，仅返回占位说明
                    content = "[secret_vault 内容已隐藏]";
                    valid = true;
                }
            } else if (embRec.memory_type == "image" || embRec.memory_type == "screenshot" || embRec.memory_type == "audio" || embRec.memory_type == "camera" || embRec.memory_type == "multimodal") {
                auto opt_memory = orm.getById<MultimodalMemory>(embRec.memory_id);
                if (opt_memory.has_value()) {
                    const auto& mm = opt_memory.value();
                    std::ostringstream oss;
                    oss << "[" << mm.data_type << "] " << mm.created_at;
                    if (!mm.description.empty()) {
                        oss << " 描述: " << mm.description;
                    }
                    if (!mm.file_path.empty()) {
                        oss << " 文件: " << mm.file_path;
                    }
                    content = oss.str();
                    valid = true;
                }
            } else {
                // 未知类型保留占位内容，避免崩溃
                content = "[未知类型: " + embRec.memory_type + "]";
                valid = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "[VectorSearchManager] 获取记忆内容失败 (" << embRec.memory_type 
                      << " ID=" << embRec.memory_id << "): " << e.what() << std::endl;
        }
        
        if (!valid) {
            missing_records++;
            continue; // 跳过无法解析或已不存在的记忆
        }
        
        result.content = content;
        
        // 计算相似度（通过余弦相似度）
        result.relevance_score = cosineSimilarity(queryEmbedding, embRec.embedding);
        result.time_score = calculateTimeScore(embRec.created_at);
        
        results.push_back(result);
        mapped_ok++;
    }
    
    // 输出详细的映射统计
    std::cout << "[VectorSearch]   已过滤其他用户: " << filtered_by_user << " 条" << std::endl;
    std::cout << "[VectorSearch]   记忆记录不存在: " << missing_records << " 条" << std::endl;
    std::cout << "[VectorSearch]   成功映射: " << mapped_ok << " 条" << std::endl;

    if (results.empty()) {
        std::cout << "\n[VectorSearch] ========== 向量检索结束 (0 结果) ==========" << std::endl;
        return results;
    }

    // 注：短期记忆由 ContextBuilder::buildRecentMessagesSection 直接从 MessageMemoryManager 获取
    // 此处不再补充 message-type 的结果，避免僵尸代码和路径混乱

    // Step 3: 计算混合得分
    std::cout << "\n[VectorSearch] -- Step 3: 计算混合得分" << std::endl;
    for (auto& result : results) {
        // 混合得分 = 相似度得分 * similarityWeight + 时间得分 * timeWeight
        result.hybrid_score = result.relevance_score * config_.similarityWeight + result.time_score * config_.timeWeight;
    }
    std::cout << "[VectorSearch]   相似度权重: " << config_.similarityWeight << std::endl;
    std::cout << "[VectorSearch]   时间权重: " << config_.timeWeight << std::endl;

    // 按混合得分降序排序
    std::sort(results.begin(), results.end(), [](const HybridSearchResult& a, const HybridSearchResult& b) {
        return a.hybrid_score > b.hybrid_score;
    });

    // Step 4: 类型分桶 + 配额控制
    std::cout << "\n[VectorSearch] -- Step 4: 类型分桶 + 配额控制" << std::endl;
    std::map<std::string, std::vector<HybridSearchResult>> type_buckets;
    for (const auto& result : results) {
        type_buckets[result.type].push_back(result);
    }
    
    // 各类型配额映射
    std::map<std::string, int> type_quotas;
    type_quotas["episodic"] = config_.maxEpisodic;
    type_quotas["semantic"] = config_.maxSemantic;
    type_quotas["core"] = config_.maxCore;
    type_quotas["procedural"] = config_.maxProcedural;
    type_quotas["multimodal"] = config_.maxMultimodal;
    type_quotas["image"] = config_.maxMultimodal;        // 多模态子类型共享配额
    type_quotas["screenshot"] = config_.maxMultimodal;
    type_quotas["audio"] = config_.maxMultimodal;
    type_quotas["camera"] = config_.maxMultimodal;
    type_quotas["message"] = limit;  // message 类型使用全局 limit
    
    // 按配额重新组装结果
    std::vector<HybridSearchResult> reranked_results;
    for (const auto& pair : type_buckets) {
        const std::string& type = pair.first;
        const std::vector<HybridSearchResult>& bucket = pair.second;
        
        int quota = type_quotas.count(type) ? type_quotas[type] : limit;
        int count = 0;
        for (const auto& result : bucket) {
            if (count >= quota) {
                std::cout << "  [" << type << "] 超过配额 " << quota << "，截断" << std::endl;
                break;
            }
            reranked_results.push_back(result);
            count++;
        }
        if (count > 0) {
            std::cout << "[VectorSearch]   [" << type << "] 保留 " << count << " 条 (配额: " 
                      << quota << ")" << std::endl;
        }
    }
    
    // 重新按 hybrid_score 排序（因为分桶后顺序被打乱）
    std::sort(reranked_results.begin(), reranked_results.end(), [](const HybridSearchResult& a, const HybridSearchResult& b) {
        return a.hybrid_score > b.hybrid_score;
    });
    
    // 用分桶后的结果替换原结果
    results = std::move(reranked_results);
    std::cout << "[VectorSearch]   分桶后总结果数: " << results.size() << std::endl;

    // 如果设置了限制，截取前N个结果（作为全局上限）
    if (limit > 0 && results.size() > static_cast<size_t>(limit)) {
        results.resize(limit);
        std::cout << "[VectorSearch]   应用全局上限: " << results.size() << std::endl;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "\n[VectorSearch] ========== 向量检索结束 (" << results.size() << " 结果, 耗时 " << duration.count() << " ms) ==========" << std::endl;

    return results;
}

std::vector<HybridSearchResult> VectorSearchManager::hybridSearch(int userId, const std::string& query, int limit) {
    std::cout << "\n[HybridSearch] ========== 混合检索开始 ==========" << std::endl;
    std::cout << "[HybridSearch] Query    : \"" << query << "\"" << std::endl;
    std::cout << "[HybridSearch] User     : " << userId << std::endl;
    std::cout << "[HybridSearch] Limit    : " << limit << std::endl;
    
    // Step 1: 生成查询 embedding
    std::cout << "\n[HybridSearch] -- Step 1: 生成查询 embedding" << std::endl;
    std::vector<float> queryEmbedding = generateTextEmbedding(query);
    std::cout << "[HybridSearch]   Embedding 维度: " << queryEmbedding.size() << std::endl;
    
    // 检查embedding是否为空
    if (queryEmbedding.empty()) {
        std::cerr << "[HybridSearch] Embedding 生成失败，返回空结果" << std::endl;
        return {};
    }
    
    // Step 2: 执行向量检索
    std::cout << "\n[HybridSearch] -- Step 2: 执行向量检索" << std::endl;
    auto results = vectorSearch(userId, queryEmbedding, limit);
    
    // Step 3: 文本 fallback（仅当向量检索为空时）
    if (results.empty()) {
        std::cout << "\n[HybridSearch] -- Step 3: 文本 fallback" << std::endl;
        
        // 基本文本搜索：在该用户的短期回忆消息中做简单关键词匹配
        std::vector<HybridSearchResult> textResults;
        try {
            auto recentMessages = messageMemoryManager_.getByUserId(userId);
            // 仅在没有任何短期记忆或长期记忆命中时，尝试按内容匹配
            auto toLower = [](const std::string& s) {
                std::string out;
                out.reserve(s.size());
                for (char c : s) {
                    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                }
                return out;
            };
            std::string queryLower = toLower(query);

            int count = 0;
            for (auto it = recentMessages.rbegin(); it != recentMessages.rend() && count < limit; ++it) {
                std::string contentLower = toLower(it->content);
                if (contentLower.find(queryLower) != std::string::npos) {
                    HybridSearchResult r;
                    r.id = it->id;
                    r.type = "message";
                    r.content = it->content;
                    r.timestamp = it->created_at;
                    // 这里不再有向量相似度，赋予一个固定的相关性得分，并依赖时间得分做排序
                    r.relevance_score = 0.8f;
                    r.time_score = calculateTimeScore(it->created_at);
                    r.hybrid_score = r.relevance_score * config_.similarityWeight + r.time_score * config_.timeWeight;
                    textResults.push_back(r);
                    ++count;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[VectorSearchManager] 基本文本搜索过程中出错: " << e.what() << std::endl;
        }

        if (!textResults.empty()) {
            std::cout << "[HybridSearch]   文本搜索命中: " << textResults.size() << " 条" << std::endl;
            results.swap(textResults);
        } else {
            std::cout << "[HybridSearch]   文本搜索未命中" << std::endl;
        }
    }
    
    std::cout << "\n[HybridSearch] ========== 混合检索结束 (" << results.size() << " 结果) ==========\n" << std::endl;
    
    // 统计信息（仅当有结果时）
    if (!results.empty()) {
        std::map<std::string, int> type_counts;
        std::map<std::string, float> type_max_scores;
        std::map<std::string, float> type_sum_scores;
        float max_hybrid_score = 0.0f;
        float sum_hybrid_score = 0.0f;
        
        for (const auto& result : results) {
            type_counts[result.type]++;
            type_sum_scores[result.type] += result.hybrid_score;
            
            if (type_max_scores.find(result.type) == type_max_scores.end() || result.hybrid_score > type_max_scores[result.type]) {
                type_max_scores[result.type] = result.hybrid_score;
            }
            
            max_hybrid_score = std::max(max_hybrid_score, result.hybrid_score);
            sum_hybrid_score += result.hybrid_score;
        }
        
        float avg_hybrid_score = sum_hybrid_score / results.size();
        
        std::cout << "\n[HybridSearch] ----- 检索质量统计 -----" << std::endl;
        std::cout << "[HybridSearch]   总结果数: " << results.size() << std::endl;
        std::cout << "[HybridSearch]   平均混合得分: " << std::fixed << std::setprecision(3) << avg_hybrid_score << std::endl;
        std::cout << "[HybridSearch]   最大混合得分: " << std::fixed << std::setprecision(3) << max_hybrid_score << std::endl;
        
        std::cout << "\n[HybridSearch]   各类型统计:" << std::endl;
        for (const auto& pair : type_counts) {
            const std::string& type = pair.first;
            int count = pair.second;
            float avg_score = type_sum_scores[type] / count;
            float max_score = type_max_scores[type];
            
            std::cout << "[HybridSearch]     - " << type << ": 数量=" << count 
                      << ", 平均得分=" << std::fixed << std::setprecision(3) << avg_score
                      << ", 最大得分=" << std::fixed << std::setprecision(3) << max_score << std::endl;
        }
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