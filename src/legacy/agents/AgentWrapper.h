#ifndef AGENT_WRAPPER_H
#define AGENT_WRAPPER_H
#include <algorithm>

// AgentWrapper - 主控代理封装，管理所有代理的生命周期

#include <string>
#include <memory>
#include "legacy/core-v1/RecallMemoryManager.h"
#include "legacy/core-v1/ArchivalMemoryManager.h"
#include "legacy/infra-alt/ModelManager.h"
#include "legacy/infra-alt/EmbeddingIndexManager.h"
#include "legacy/agents/CoreMemoryAgent.h"
#include "legacy/agents/EpisodicMemoryAgent.h"
#include "legacy/agents/SemanticMemoryAgent.h"
#include "legacy/agents/ProceduralMemoryAgent.h"
#include "legacy/agents/ResourceMemoryAgent.h"
#include "legacy/agents/KnowledgeVaultAgent.h"

class AgentWrapper {
private:
    StorageManager& storage_;
    ModelManager& model_;
    
    // 代理实例
    std::unique_ptr<CoreMemoryAgent> coreMemoryAgent_;
    std::unique_ptr<EpisodicMemoryAgent> episodicMemoryAgent_;
    std::unique_ptr<SemanticMemoryAgent> semanticMemoryAgent_;
    std::unique_ptr<ProceduralMemoryAgent> proceduralMemoryAgent_;
    std::unique_ptr<ResourceMemoryAgent> resourceMemoryAgent_;
    std::unique_ptr<KnowledgeVaultAgent> knowledgeVaultAgent_;

public:
    // 构造函数
    AgentWrapper(StorageManager& storage, ModelManager& model);
    
    // 核心代理接口
    std::string processQuery(int userId, const std::string& query);
    bool storeMemory(int userId, int sessionId, int agentId, 
                    const std::string& content, const std::string& metadata = "");
    
    // 代理获取接口
    CoreMemoryAgent* get_core_memory_agent();
    EpisodicMemoryAgent* get_episodic_memory_agent();
    SemanticMemoryAgent* get_semantic_memory_agent();
    ProceduralMemoryAgent* get_procedural_memory_agent();
    ResourceMemoryAgent* get_resource_memory_agent();
    KnowledgeVaultAgent* get_knowledge_vault_agent();
};

#endif // AGENT_WRAPPER_H