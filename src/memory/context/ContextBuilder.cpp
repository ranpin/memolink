#include "ContextBuilder.h"
#include "../../database/models/MemoryModels.h"
#include "../../database/managers/MessageMemoryManager.h"
#include "../../database/managers/EpisodicMemoryManager.h"
#include "../../database/managers/SemanticMemoryManager.h"
#include "../../database/managers/CoreMemoryBlockManager.h"
#include "../../database/managers/ProceduralMemoryManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// 实现 ContextBuilder 类的方法

HierarchicalContext ContextBuilder::buildContext(int user_id, const std::string& query, 
                                               MemoryOrchestrator& orchestrator,
                                               VectorSearchManager* vector_search,
                                               int session_id) {
    HierarchicalContext context;
    
    // 构建最近消息部分（传递 session_id 进行过滤）
    context.recent_messages = buildRecentMessagesSection(user_id, orchestrator, session_id);
    
    // 【R1优化】统一一次混合向量搜索，避免重复检索
    std::vector<HybridSearchResult> all_search_results;
    if (vector_search && !query.empty()) {
        std::cout << "[ContextBuilder] 执行统一混合向量搜索（一次检索所有长期记忆）..." << std::endl;
        // 一次性检索所有长期记忆类型，limit 设置为所有类型配额之和的 1.5 倍以保证覆盖
        // maxEpisodic=4 + maxSemantic=4 + maxCore=3 + maxProcedural=3 + maxMultimodal=4 = 18，取 30 作为上限
        all_search_results = vector_search->hybridSearch(user_id, query, 30);
        std::cout << "[ContextBuilder] 统一检索返回 " << all_search_results.size() << " 条结果" << std::endl;
    }
    
    // 构建各类长期记忆部分（传递统一检索结果）
    context.episodic_memories = buildEpisodicMemoriesSection(user_id, query, orchestrator, vector_search, all_search_results);
    context.semantic_memories = buildSemanticMemoriesSection(user_id, query, orchestrator, vector_search, all_search_results);
    context.core_memories = buildCoreMemoriesSection(user_id, query, orchestrator, vector_search, all_search_results);
    context.procedural_memories = buildProceduralMemoriesSection(user_id, query, orchestrator, vector_search, all_search_results);
    context.multimodal_memories = buildMultimodalMemoriesSection(user_id, query, orchestrator, vector_search, all_search_results);
    
    // 输出汇总日志
    std::cout << "\n[ContextBuilder] ========== 上下文构建完成 ==========" << std::endl;
    std::cout << "[ContextBuilder] User    : " << user_id << std::endl;
    std::cout << "[ContextBuilder] Session : " << session_id << std::endl;
    std::cout << "[ContextBuilder] Query   : \"" << query << "\"" << std::endl;
    std::cout << "\n[ContextBuilder] 各区块记忆数量:" << std::endl;
    std::cout << "[ContextBuilder]   - 最近消息     : " << context.recent_messages.size() << " 条" << std::endl;
    std::cout << "[ContextBuilder]   - 情景记忆     : " << context.episodic_memories.size() << " 条" << std::endl;
    std::cout << "[ContextBuilder]   - 语义记忆     : " << context.semantic_memories.size() << " 条" << std::endl;
    std::cout << "[ContextBuilder]   - 核心记忆     : " << context.core_memories.size() << " 条" << std::endl;
    std::cout << "[ContextBuilder]   - 程序记忆     : " << context.procedural_memories.size() << " 条" << std::endl;
    std::cout << "[ContextBuilder]   - 多模态记忆   : " << context.multimodal_memories.size() << " 条" << std::endl;
    
    int total_memories = context.episodic_memories.size() + context.semantic_memories.size() + 
                         context.core_memories.size() + context.procedural_memories.size() + 
                         context.multimodal_memories.size();
    std::cout << "[ContextBuilder]   总计: " << context.recent_messages.size() << " 条消息 + " 
              << total_memories << " 条长期记忆" << std::endl;
    
    return context;
}

std::string ContextBuilder::formatContext(const HierarchicalContext& context, int max_tokens) {
    std::ostringstream formatted_context;

    // 将 max_tokens 作为近似的字符上限，避免上下文过长
    const int max_chars = max_tokens > 0 ? max_tokens : 0;
    int current_length = 0;

    auto append_text = [&](const std::string& text) {
        if (max_chars == 0) {
            // 未设置上限时，直接追加全部内容
            formatted_context << text;
            return;
        }

        if (current_length >= max_chars) {
            return;
        }

        int remaining = max_chars - current_length;
        if (static_cast<int>(text.size()) <= remaining) {
            formatted_context << text;
            current_length += static_cast<int>(text.size());
        } else if (remaining > 0) {
            formatted_context << text.substr(0, remaining);
            current_length += remaining;
        }
    };

    auto append_section = [&](const std::string& header,
                              const std::vector<HierarchicalContext::ContextSection>& sections,
                              bool sort_by_priority) {
        if (max_chars > 0 && current_length >= max_chars) {
            return;
        }

        append_text(header);
        if (max_chars > 0 && current_length >= max_chars) {
            return;
        }

        std::vector<HierarchicalContext::ContextSection> sorted_sections = sections;
        if (sort_by_priority) {
            std::sort(sorted_sections.begin(), sorted_sections.end(),
                      [](const HierarchicalContext::ContextSection& a, const HierarchicalContext::ContextSection& b) {
                          return a.priority > b.priority;
                      });
        }

        for (const auto& section : sorted_sections) {
            if (max_chars > 0 && current_length >= max_chars) {
                break;
            }
            append_text(section.content);
            append_text("\n");
        }
    };

    // 优先保留最近消息
    append_section("=== 最近消息 ===\n", context.recent_messages, false);

    // 然后保留最相关的多模态记忆（按 priority 降序）
    append_section("\n=== 多模态记忆 ===\n", context.multimodal_memories, true);

    // 其余长期记忆按原有顺序追加，在空间不足时会优先被截断
    append_section("\n=== 情景记忆 ===\n", context.episodic_memories, true);
    append_section("\n=== 语义记忆 ===\n", context.semantic_memories, true);
    append_section("\n=== 核心记忆 ===\n", context.core_memories, true);
    append_section("\n=== 程序记忆 ===\n", context.procedural_memories, true);

    return formatted_context.str();
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildRecentMessagesSection(
    int user_id, MemoryOrchestrator& orchestrator, int session_id) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        auto& manager = orchestrator.getMessageMemoryManager();
        auto messages = manager.getAll(); // 使用getAll获取所有消息，然后过滤
        
        // 过滤指定用户的消息，如果 session_id > 0 则额外过滤 session
        std::vector<MessageMemory> user_messages;
        for (const auto& msg : messages) {
            bool match_user = (msg.user_id == user_id);
            bool match_session = (session_id <= 0 || msg.session_id == session_id);
            if (match_user && match_session) {
                user_messages.push_back(msg);
            }
        }
        
        // 限制消息数量
        int count = 0;
        for (const auto& msg : user_messages) {
            if (count >= 10) break; // 限制最近10条消息
            
            HierarchicalContext::ContextSection section;
            section.title = "最近消息";
            section.content = msg.content;
            section.relevance_score = 1.0f; // 最近消息相关性最高
            section.priority = 100 - count; // 越近的消息优先级越高
            section.type = "recent_messages";
            
            sections.push_back(section);
            count++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building recent messages section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildEpisodicMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
    VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        // 【R1优化】从统一检索结果中过滤 episodic 类型
        if (vector_search && !query.empty() && !all_search_results.empty()) {
            
            for (const auto& result : all_search_results) {
                if (result.type == "episodic") {
                    // 过滤掉混合得分过低的结果，避免低相关记忆占用上下文
                    if (result.hybrid_score < 0.2f) {
                        continue;
                    }
                    HierarchicalContext::ContextSection section;
                    section.title = "情景记忆";
                    section.content = result.content;
                    section.relevance_score = result.relevance_score;
                    section.priority = static_cast<int>(result.hybrid_score * 100);
                    section.type = "episodic_memory";
                    sections.push_back(section);
                }
            }
            
            std::cout << "[ContextBuilder] 过滤到 " << sections.size() << " 条情景记忆" << std::endl;
            
            if (!sections.empty()) {
                return sections; // 如果过滤到结果，直接返回
            }
        }
        
        // 如果没有 VectorSearchManager 或检索无结果，使用传统方法
        std::cout << "[ContextBuilder] 使用 fallback 获取情景记忆 (" << sections.size() << " 条)" << std::endl;
        auto& manager = orchestrator.getEpisodicMemoryManager();
        auto memories = manager.getAll(); // 使用getAll获取所有记忆，然后过滤
        
        // 过滤指定用户的情景记忆
        std::vector<EpisodicMemory> user_memories;
        for (const auto& memory : memories) {
            if (memory.user_id == user_id) {
                user_memories.push_back(memory);
            }
        }
        
        // Fallback 优化：简单关键词匹配 + 时间排序
        // 提取查询中的关键词（长度>=2的词）
        std::vector<std::string> keywords;
        std::string word;
        for (char c : query) {
            if (std::isalnum(c) || c == '_' || (c & 0x80)) { // 支持中文字符
                word += c;
            } else {
                if (word.length() >= 2) {
                    keywords.push_back(word);
                }
                word.clear();
            }
        }
        if (word.length() >= 2) {
            keywords.push_back(word);
        }
        
        // 为每条记忆计算匹配得分
        struct ScoredMemory {
            EpisodicMemory memory;
            float score;
        };
        std::vector<ScoredMemory> scored_memories;
        
        for (const auto& memory : user_memories) {
            float score = 0.0f;
            // 关键词匹配得分
            for (const auto& keyword : keywords) {
                if (memory.event_description.find(keyword) != std::string::npos) {
                    score += 1.0f;
                }
            }
            // 时间新近度（简化版：ID越大越新，给予加成）
            score += memory.id * 0.001f;
            scored_memories.push_back({memory, score});
        }
        
        // 按得分降序排序
        std::sort(scored_memories.begin(), scored_memories.end(), 
                  [](const ScoredMemory& a, const ScoredMemory& b) {
                      return a.score > b.score;
                  });
        
        // 限制记忆数量
        int count = 0;
        for (const auto& scored : scored_memories) {
            if (count >= 5) break; // 限制5条情景记忆
            
            HierarchicalContext::ContextSection section;
            section.title = "情景记忆";
            section.content = scored.memory.event_description;
            // 根据得分调整相关性
            section.relevance_score = std::min(0.8f, 0.3f + scored.score * 0.1f);
            section.priority = 80 - count;
            section.type = "episodic_memory";
            
            sections.push_back(section);
            count++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building episodic memories section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildSemanticMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
    VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        // 【R1优化】从统一检索结果中过滤 semantic 类型
        if (vector_search && !query.empty() && !all_search_results.empty()) {
            std::cout << "[ContextBuilder] 从统一检索结果中过滤语义记忆..." << std::endl;
            
            for (const auto& result : all_search_results) {
                if (result.type == "semantic") {
                    // 过滤掉混合得分过低的结果，避免低相关记忆占用上下文
                    if (result.hybrid_score < 0.2f) {
                        continue;
                    }
                    HierarchicalContext::ContextSection section;
                    section.title = "语义记忆";
                    section.content = result.content;
                    section.relevance_score = result.relevance_score;
                    section.priority = static_cast<int>(result.hybrid_score * 100);
                    section.type = "semantic_memory";
                    sections.push_back(section);
                }
            }
            
            std::cout << "[ContextBuilder] 过滤到 " << sections.size() << " 条语义记忆" << std::endl;
            
            if (!sections.empty()) {
                return sections;
            }
        }
        
        // 如果没有 VectorSearchManager 或检索无结果，使用传统方法
        std::cout << "[ContextBuilder] 使用传统方法获取语义记忆..." << std::endl;
        auto& manager = orchestrator.getSemanticMemoryManager();
        auto memories = manager.getAll(); // 使用getAll获取所有记忆，然后过滤
        
        // 过滤指定用户的语义记忆
        std::vector<SemanticMemory> user_memories;
        for (const auto& memory : memories) {
            if (memory.user_id == user_id) {
                user_memories.push_back(memory);
            }
        }
        
        // Fallback 优化：关键词匹配 + 时间排序
        std::vector<std::string> keywords;
        std::string word;
        for (char c : query) {
            if (std::isalnum(c) || c == '_' || (c & 0x80)) {
                word += c;
            } else {
                if (word.length() >= 2) keywords.push_back(word);
                word.clear();
            }
        }
        if (word.length() >= 2) keywords.push_back(word);
        
        struct ScoredMemory {
            SemanticMemory memory;
            float score;
        };
        std::vector<ScoredMemory> scored_memories;
        
        for (const auto& memory : user_memories) {
            float score = 0.0f;
            for (const auto& keyword : keywords) {
                if (memory.fact.find(keyword) != std::string::npos) {
                    score += 1.0f;
                }
            }
            score += memory.id * 0.001f;
            scored_memories.push_back({memory, score});
        }
        
        std::sort(scored_memories.begin(), scored_memories.end(), 
                  [](const ScoredMemory& a, const ScoredMemory& b) {
                      return a.score > b.score;
                  });
        
        // 限制记忆数量
        int count = 0;
        for (const auto& scored : scored_memories) {
            if (count >= 5) break; // 限制5条语义记忆
            
            HierarchicalContext::ContextSection section;
            section.title = "语义记忆";
            section.content = scored.memory.fact;
            section.relevance_score = std::min(0.7f, 0.3f + scored.score * 0.1f);
            section.priority = 70 - count;
            section.type = "semantic_memory";
            
            sections.push_back(section);
            count++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building semantic memories section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildCoreMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
    VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        // 【R1优化】从统一检索结果中过滤 core 类型
        if (vector_search && !query.empty() && !all_search_results.empty()) {
            std::cout << "[ContextBuilder] 从统一检索结果中过滤核心记忆..." << std::endl;
            
            for (const auto& result : all_search_results) {
                if (result.type == "core") {
                    // 过滤掉混合得分过低的结果，避免低相关记忆占用上下文
                    if (result.hybrid_score < 0.2f) {
                        continue;
                    }
                    HierarchicalContext::ContextSection section;
                    section.title = "核心记忆";
                    section.content = result.content;
                    section.relevance_score = result.relevance_score;
                    section.priority = static_cast<int>(result.hybrid_score * 100);
                    section.type = "core_memory";
                    sections.push_back(section);
                }
            }
            
            std::cout << "[ContextBuilder] 过滤到 " << sections.size() << " 条核心记忆" << std::endl;
            if (!sections.empty()) {
                return sections;
            }
        }
        
        // 如果没有 VectorSearchManager 或检索无结果，使用传统方法
        std::cout << "[ContextBuilder] 使用传统方法获取核心记忆..." << std::endl;
        auto& manager = orchestrator.getCoreMemoryBlockManager();
        auto memories = manager.getAll(); // 使用getAll获取所有记忆，然后过滤
        
        // 过滤指定用户的核心记忆
        std::vector<CoreMemory> user_memories;
        for (const auto& memory : memories) {
            if (memory.user_id == user_id) {
                user_memories.push_back(memory);
            }
        }
        
        // Fallback 优化：关键词匹配 + 时间排序
        std::vector<std::string> keywords;
        std::string word;
        for (char c : query) {
            if (std::isalnum(c) || c == '_' || (c & 0x80)) {
                word += c;
            } else {
                if (word.length() >= 2) keywords.push_back(word);
                word.clear();
            }
        }
        if (word.length() >= 2) keywords.push_back(word);
        
        struct ScoredMemory {
            CoreMemory memory;
            float score;
        };
        std::vector<ScoredMemory> scored_memories;
        
        for (const auto& memory : user_memories) {
            float score = 0.0f;
            for (const auto& keyword : keywords) {
                if (memory.content.find(keyword) != std::string::npos) {
                    score += 1.0f;
                }
            }
            score += memory.id * 0.001f;
            scored_memories.push_back({memory, score});
        }
        
        std::sort(scored_memories.begin(), scored_memories.end(), 
                  [](const ScoredMemory& a, const ScoredMemory& b) {
                      return a.score > b.score;
                  });
        
        // 限制记忆数量
        int count = 0;
        for (const auto& scored : scored_memories) {
            if (count >= 3) break; // 限制3条核心记忆
            
            HierarchicalContext::ContextSection section;
            section.title = "核心记忆";
            section.content = scored.memory.content;
            section.relevance_score = std::min(0.9f, 0.4f + scored.score * 0.1f);
            section.priority = 90 - count;
            section.type = "core_memory";
            
            sections.push_back(section);
            count++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building core memories section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildProceduralMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
    VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        // 【R1优化】从统一检索结果中过滤 procedural 类型
        if (vector_search && !query.empty() && !all_search_results.empty()) {
            std::cout << "[ContextBuilder] 从统一检索结果中过滤程序记忆..." << std::endl;
            
            for (const auto& result : all_search_results) {
                if (result.type == "procedural") {
                    // 过滤掉混合得分过低的结果，避免低相关记忆占用上下文
                    if (result.hybrid_score < 0.2f) {
                        continue;
                    }
                    HierarchicalContext::ContextSection section;
                    section.title = "程序记忆";
                    section.content = result.content;
                    section.relevance_score = result.relevance_score;
                    section.priority = static_cast<int>(result.hybrid_score * 100);
                    section.type = "procedural_memory";
                    sections.push_back(section);
                }
            }
            
            std::cout << "[ContextBuilder] 过滤到 " << sections.size() << " 条程序记忆" << std::endl;
            
            if (!sections.empty()) {
                return sections;
            }
        }
        
        // 如果没有 VectorSearchManager 或检索无结果，使用传统方法
        std::cout << "[ContextBuilder] 使用传统方法获取程序记忆..." << std::endl;
        auto& manager = orchestrator.getProceduralMemoryManager();
        auto memories = manager.getAll(); // 使用getAll获取所有记忆，然后过滤
        
        // 过滤指定用户的程序记忆
        std::vector<ProceduralMemory> user_memories;
        for (const auto& memory : memories) {
            if (memory.user_id == user_id) {
                user_memories.push_back(memory);
            }
        }
        
        // Fallback 优化：关键词匹配 + 时间排序
        std::vector<std::string> keywords;
        std::string word;
        for (char c : query) {
            if (std::isalnum(c) || c == '_' || (c & 0x80)) {
                word += c;
            } else {
                if (word.length() >= 2) keywords.push_back(word);
                word.clear();
            }
        }
        if (word.length() >= 2) keywords.push_back(word);
        
        struct ScoredMemory {
            ProceduralMemory memory;
            float score;
        };
        std::vector<ScoredMemory> scored_memories;
        
        for (const auto& memory : user_memories) {
            float score = 0.0f;
            for (const auto& keyword : keywords) {
                if (memory.description.find(keyword) != std::string::npos) {
                    score += 1.0f;
                }
            }
            score += memory.id * 0.001f;
            scored_memories.push_back({memory, score});
        }
        
        std::sort(scored_memories.begin(), scored_memories.end(), 
                  [](const ScoredMemory& a, const ScoredMemory& b) {
                      return a.score > b.score;
                  });
        
        // 限制记忆数量
        int count = 0;
        for (const auto& scored : scored_memories) {
            if (count >= 3) break; // 限制3条程序记忆
            
            HierarchicalContext::ContextSection section;
            section.title = "程序记忆";
            section.content = scored.memory.description;
            section.relevance_score = std::min(0.6f, 0.3f + scored.score * 0.1f);
            section.priority = 60 - count;
            section.type = "procedural_memory";
            
            sections.push_back(section);
            count++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building procedural memories section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildMultimodalMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
    VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results) {
    std::vector<HierarchicalContext::ContextSection> sections;
    
    try {
        // 【R1优化】从统一检索结果中过滤多模态类型
        if (vector_search && !query.empty() && !all_search_results.empty()) {
            std::cout << "[ContextBuilder] 从统一检索结果中过滤多模态记忆..." << std::endl;
            
            for (const auto& result : all_search_results) {
                if (result.type == "image" || result.type == "screenshot" || result.type == "audio" ||
                    result.type == "camera" || result.type == "multimodal") {
                    // 过滤掉混合得分过低的结果，避免低相关记忆占用上下文
                    if (result.hybrid_score < 0.2f) {
                        continue;
                    }
                    HierarchicalContext::ContextSection section;
                    section.title = "多模态记忆";
                    section.content = "- [" + result.type + "] " + result.content;
                    section.relevance_score = result.relevance_score;
                    section.priority = static_cast<int>(result.hybrid_score * 100);
                    section.type = "multimodal_memory";
                    sections.push_back(section);
                }
            }
            
            std::cout << "[ContextBuilder] 过滤到 " << sections.size() << " 条多模态记忆" << std::endl;
        }
        
        // Fallback: 当没有向量检索或检索无结果时，从数据库按时间倒序获取最近的多模态记忆
        if (sections.empty()) {
            std::cout << "[ContextBuilder] 向量检索无结果，使用数据库 fallback 获取最近多模态记忆..." << std::endl;
            
            // 通过 ORMWrapper 获取最近的多模态记忆
            auto& orm_wrapper = orchestrator.getORMWrapper();
            auto multimodal_memories = orm_wrapper.getMultimodalMemoriesByUserId(user_id, 5);
            
            int count = 0;
            for (const auto& mm : multimodal_memories) {
                HierarchicalContext::ContextSection section;
                section.title = "多模态记忆";
                
                // 格式化多模态记忆内容：[类型] 描述 (时间)
                section.content = "- [" + mm.data_type + "] " + mm.description + " (" + mm.created_at + ")";
                section.relevance_score = 0.5f;  // fallback 的相关性评分设为中等
                section.priority = 50 - count;   // 按时间倒序，越新的优先级越高
                section.type = "multimodal_memory";
                
                sections.push_back(section);
                count++;
            }
            
            std::cout << "[ContextBuilder] 通过数据库 fallback 获取了 " << sections.size() << " 条多模态记忆" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error building multimodal memories section: " << e.what() << std::endl;
    }
    
    return sections;
}

std::vector<HierarchicalContext::ContextSection> ContextBuilder::buildResourceMemoriesSection(
    int user_id, const std::string& query, MemoryOrchestrator& orchestrator) {
    std::vector<HierarchicalContext::ContextSection> sections;
    // ResourceMemory已被删除，返回空结果
    return sections;
}
