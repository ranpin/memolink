#pragma once
#include <algorithm>

#include "../database/core/StorageManager.h"
#include "../database/managers/UserManager.h"
#include "../database/managers/SessionManager.h"
#include "../agents/AgentManager.h"
#include "../database/managers/RecallMemoryManager.h"
#include "../database/managers/CoreMemoryBlockManager.h"
#include "../database/managers/EpisodicMemoryManager.h"
#include "../database/managers/SemanticMemoryManager.h"
#include "../database/managers/ProceduralMemoryManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../database/managers/KnowledgeVaultManager.h"
#include "../database/managers/ArchivalMemoryManager.h"
#include "../models/ModelManager.h"
#include "../agents/AgentWrapper.h"
#include <string>
#include <vector>
#include <memory>

class APIServer {
private:
    StorageManager& storage_;
    std::unique_ptr<UserManager> userManager_;
    std::unique_ptr<SessionManager> sessionManager_;
    std::unique_ptr<AgentManager> agentManager_;
    std::unique_ptr<RecallMemoryManager> recallMemoryManager_;
    std::unique_ptr<CoreMemoryBlockManager> coreMemoryManager_;
    std::unique_ptr<EpisodicMemoryManager> episodicMemoryManager_;
    std::unique_ptr<SemanticMemoryManager> semanticMemoryManager_;
    std::unique_ptr<ProceduralMemoryManager> proceduralMemoryManager_;
    std::unique_ptr<ResourceMemoryManager> resourceMemoryManager_;
    std::unique_ptr<KnowledgeVaultManager> knowledgeVaultManager_;
    std::unique_ptr<ArchivalMemoryManager> archivalMemoryManager_;
    std::unique_ptr<ModelManager> modelManager_;
    std::unique_ptr<AgentWrapper> agentWrapper_;

public:
    explicit APIServer(StorageManager& storage);
    
    // 用户管理API
    int create_user(const std::string& name, const std::string& email);
    bool update_user(int user_id, const std::string& name, const std::string& email);
    bool delete_user(int user_id);
    std::vector<int> list_users(); // 返回用户ID列表
    
    // 会话管理API
    int create_session(int user_id);
    bool end_session(int session_id);
    std::vector<SessionItem> list_sessions(int user_id);
    
    // 回忆记忆API
    int add_recall_memory(int user_id, int session_id, int agent_id, 
                         const std::string& message_type, const std::string& content, 
                         const std::string& metadata);
    std::vector<RecallMemoryManager::RecallMemoryItem> search_fulltext(const std::string& user_id, 
                                                                      const std::string& query);
    std::vector<RecallMemoryManager::RecallMemoryItem> search_embedding(const std::string& user_id, 
                                                                       const std::string& query);
    std::vector<RecallMemoryManager::RecallMemoryItem> search_hybrid(const std::string& user_id,
                                                                    const std::string& query);
    
    // 核心记忆API
    int add_core_memory(int user_id, const std::string& key, const std::string& value, 
                       const std::string& description);
    bool update_core_memory(int memory_id, const std::string& key, const std::string& value, 
                           const std::string& description);
    bool delete_core_memory(int memory_id);
    std::vector<CoreMemoryBlockRecord> list_core_memories(int user_id);
    
    // 情景记忆API
    int add_episodic_memory(int user_id, int agent_id, const std::string& event, 
                           const std::string& content, const std::string& event_type, 
                           const std::string& path, const std::string& metadata);
    bool update_episodic_memory(int memory_id, const std::string& event, 
                               const std::string& content, const std::string& event_type, 
                               const std::string& path, const std::string& metadata);
    bool delete_episodic_memory(int memory_id);
    std::vector<EpisodicMemoryRecord> list_episodic_memories(int user_id);
    
    // 语义记忆API
    int add_semantic_memory(int user_id, int agent_id, const std::string& title, 
                           const std::string& content, const std::string& description, 
                           const std::string& path, const std::string& metadata);
    bool update_semantic_memory(int memory_id, const std::string& title, 
                               const std::string& content, const std::string& description, 
                               const std::string& path, const std::string& metadata);
    bool delete_semantic_memory(int memory_id);
    std::vector<SemanticMemoryRecord> list_semantic_memories(int user_id);
    
    // 程序性记忆API
    int add_procedural_memory(int user_id, int agent_id, const std::string& title, 
                             const std::string& description, const std::string& steps, 
                             const std::string& path, const std::string& metadata);
    bool update_procedural_memory(int memory_id, const std::string& title, 
                                 const std::string& description, const std::string& steps, 
                                 const std::string& path, const std::string& metadata);
    bool delete_procedural_memory(int memory_id);
    std::vector<ProceduralMemoryRecord> list_procedural_memories(int user_id);
    
    // 资源记忆API
    int add_resource_memory(int user_id, int agent_id, const std::string& title, 
                           const std::string& content, const std::string& resource_type, 
                           const std::string& path, const std::string& metadata);
    bool update_resource_memory(int memory_id, const std::string& title, 
                               const std::string& content, const std::string& resource_type, 
                               const std::string& path, const std::string& metadata);
    bool delete_resource_memory(int memory_id);
    std::vector<ResourceMemoryManager::ResourceMemoryItem> list_resource_memories(int user_id);
    
    // 知识库API
    int add_knowledge_vault_entry(int user_id, int agent_id, const std::string& title, 
                                 const std::string& content, const std::string& category, 
                                 const std::string& path, const std::string& metadata);
    bool update_knowledge_vault_entry(int entry_id, const std::string& title, 
                                     const std::string& content, const std::string& category, 
                                     const std::string& path, const std::string& metadata);
    bool delete_knowledge_vault_entry(int entry_id);
    std::vector<KnowledgeVaultRecord> list_knowledge_vault_entries(int user_id);
    
    // 归档记忆API
    int add_archival_memory(int user_id, int agent_id, const std::string& memory_type, 
                           const std::string& content, const std::string& metadata);
    bool update_archival_memory(int memory_id, const std::string& memory_type, 
                               const std::string& content, const std::string& metadata);
    bool delete_archival_memory(int memory_id);
    std::vector<ArchivalMemoryRecord> list_archival_memories(int user_id);
    
    // 模型API
    std::string chat_completion(int user_id, int session_id, int agent_id, const std::string& message);
    std::string multimodal_completion(int user_id, int session_id, int agent_id, 
                                     const std::string& text, const std::string& image_path);
    
    // 协调搜索API
    std::vector<RecallMemoryManager::RecallMemoryItem> coordinated_search(const std::string& user_id,
                                                                         const std::string& query);
};