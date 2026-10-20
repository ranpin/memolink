#include <string>
#include <memory>
#include "AgentWrapper.h"
#include "ChatAgent.h"
#include <iostream>

AgentWrapper::AgentWrapper(StorageManager& storage, ModelManager& model) 
    : storage_(storage), model_(model) {
    // 初始化各个代理
    coreMemoryAgent_ = std::make_unique<CoreMemoryAgent>(storage);
    episodicMemoryAgent_ = std::make_unique<EpisodicMemoryAgent>(storage);
    semanticMemoryAgent_ = std::make_unique<SemanticMemoryAgent>(storage);
    proceduralMemoryAgent_ = std::make_unique<ProceduralMemoryAgent>(storage);
    resourceMemoryAgent_ = std::make_unique<ResourceMemoryAgent>(storage);
    knowledgeVaultAgent_ = std::make_unique<KnowledgeVaultAgent>(storage);
}

std::string AgentWrapper::processQuery(int userId, const std::string& query) {
    // 简单实现：返回一个模拟的响应
    std::string response = "Processed query for user " + std::to_string(userId) + ": " + query;
    std::cout << "AgentWrapper processing query: " << query << std::endl;
    return response;
}

bool AgentWrapper::storeMemory(int userId, int sessionId, int agentId, 
                              const std::string& content, const std::string& metadata) {
    // 简单实现：输出存储信息
    std::cout << "Storing memory for user " << userId << ", session " << sessionId 
              << ", agent " << agentId << std::endl;
    std::cout << "Content: " << content << std::endl;
    std::cout << "Metadata: " << metadata << std::endl;
    return true;
}

CoreMemoryAgent* AgentWrapper::get_core_memory_agent() {
    return coreMemoryAgent_.get();
}

EpisodicMemoryAgent* AgentWrapper::get_episodic_memory_agent() {
    return episodicMemoryAgent_.get();
}

SemanticMemoryAgent* AgentWrapper::get_semantic_memory_agent() {
    return semanticMemoryAgent_.get();
}

ProceduralMemoryAgent* AgentWrapper::get_procedural_memory_agent() {
    return proceduralMemoryAgent_.get();
}

ResourceMemoryAgent* AgentWrapper::get_resource_memory_agent() {
    return resourceMemoryAgent_.get();
}

KnowledgeVaultAgent* AgentWrapper::get_knowledge_vault_agent() {
    return knowledgeVaultAgent_.get();
}