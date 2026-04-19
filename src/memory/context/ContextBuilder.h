#ifndef CONTEXT_BUILDER_H
#define CONTEXT_BUILDER_H

#include "../../database/managers/MemoryOrchestrator.h"
#include "../messages/MessageManager.h"
#include "../../models/VectorSearchManager.h"
#include <string>
#include <vector>
#include <memory>

struct HierarchicalContext {
    struct ContextSection {
        std::string title;
        std::string content;
        float relevance_score;
        int priority;
        std::string type; // recent_messages, episodic_memory, semantic_memory等
    };
    
    std::vector<ContextSection> recent_messages;
    std::vector<ContextSection> episodic_memories;
    std::vector<ContextSection> semantic_memories;
    std::vector<ContextSection> core_memories;
    std::vector<ContextSection> procedural_memories;
    std::vector<ContextSection> multimodal_memories;
    std::vector<ContextSection> resource_memories;
};

class ContextBuilder {
public:
    static HierarchicalContext buildContext(int user_id, const std::string& query, 
                                          MemoryOrchestrator& orchestrator,
                                          VectorSearchManager* vector_search = nullptr,
                                          int session_id = -1);
    
    static std::string formatContext(const HierarchicalContext& context, int max_tokens = 2048);

private:
    static std::vector<HierarchicalContext::ContextSection> buildRecentMessagesSection(
        int user_id, MemoryOrchestrator& orchestrator, int session_id = -1);
    
    static std::vector<HierarchicalContext::ContextSection> buildEpisodicMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
        VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results);
    
    static std::vector<HierarchicalContext::ContextSection> buildSemanticMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
        VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results);
    
    static std::vector<HierarchicalContext::ContextSection> buildCoreMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
        VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results);
    
    static std::vector<HierarchicalContext::ContextSection> buildProceduralMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
        VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results);
    
    static std::vector<HierarchicalContext::ContextSection> buildMultimodalMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator,
        VectorSearchManager* vector_search, const std::vector<HybridSearchResult>& all_search_results);
    
    static std::vector<HierarchicalContext::ContextSection> buildResourceMemoriesSection(
        int user_id, const std::string& query, MemoryOrchestrator& orchestrator);
};

#endif // CONTEXT_BUILDER_H