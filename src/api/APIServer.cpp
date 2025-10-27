#include "APIServer.h"
#include "../database/managers/UserManager.h"
#include "../agents/AgentManager.h"
#include "../database/managers/SessionManager.h"
#include "../database/managers/CoreMemoryBlockManager.h"
#include "../database/managers/EpisodicMemoryManager.h"
#include "../database/managers/SemanticMemoryManager.h"
#include "../database/managers/ProceduralMemoryManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../database/managers/KnowledgeVaultManager.h"
#include "../database/managers/ArchivalMemoryManager.h"
#include "../database/managers/RecallMemoryManager.h"
#include "../models/ModelManager.h"
#include <iostream>
#include <memory>
#include <string>

APIServer::APIServer(StorageManager& storage) 
    : storage_(storage),
      userManager_(std::make_unique<UserManager>()),
      sessionManager_(std::make_unique<SessionManager>(storage)),
      coreMemoryManager_(std::make_unique<CoreMemoryBlockManager>(storage)),
      resourceMemoryManager_(std::make_unique<ResourceMemoryManager>(storage)),
      knowledgeVaultManager_(std::make_unique<KnowledgeVaultManager>(storage)),
      recallMemoryManager_(std::make_unique<RecallMemoryManager>(storage)),
      modelManager_(std::make_unique<ModelManager>(storage)) {
    // 注意：AgentManager需要ModelManager，这里暂时不初始化
    // agentManager_需要在其他地方初始化
    // episodicMemoryManager_等静态方法类也不需要初始化
}

// 用户管理API
int APIServer::create_user(const std::string& name, const std::string& email) {
    // UserManager是静态方法类，不需要实例化
    // 这里需要一个密码参数，但接口没有提供，我们使用一个默认值
    bool success = UserManager::addUser(name, email, "default_password", "./db.sqlite");
    if (success) {
        return UserManager::getUserIdByName(name, "./db.sqlite");
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_user(int user_id, const std::string& name, const std::string& email) {
    // 这里需要实现更新用户的方法，但UserManager没有提供
    return false;
}

bool APIServer::delete_user(int user_id) {
    // UserManager没有提供删除用户的方法
    return false;
}

std::vector<int> APIServer::list_users() {
    // UserManager没有提供list_users方法
    return std::vector<int>();
}

// 会话管理API
int APIServer::create_session(int user_id) {
    SessionItem session;
    session.user_id = std::to_string(user_id);
    session.token = "token_" + std::to_string(user_id); // 简化的令牌生成
    session.status = "active";
    session.device_info = "default_device";
    
    bool success = sessionManager_->create_session(session);
    if (success) {
        return session.id;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::end_session(int session_id) {
    return sessionManager_->end_session(session_id);
}

std::vector<SessionItem> APIServer::list_sessions(int user_id) {
    return sessionManager_->list_sessions(std::to_string(user_id));
}

// 回忆记忆API
int APIServer::add_recall_memory(int user_id, int session_id, int agent_id, 
                                const std::string& message_type, const std::string& content, 
                                const std::string& metadata) {
    RecallMemoryManager::RecallMemoryItem item;
    item.user_id = user_id;
    item.agent_id = agent_id;
    item.message_type = message_type;
    item.content = content;
    item.metadata = metadata;
    
    bool success = recallMemoryManager_->add_recall_memory(item);
    if (success) {
        return item.id;
    } else {
        return -1; // 表示失败
    }
}

std::vector<RecallMemoryManager::RecallMemoryItem> APIServer::search_fulltext(const std::string& user_id, 
                                                                           const std::string& query) {
    // 这里需要实现搜索功能
    return std::vector<RecallMemoryManager::RecallMemoryItem>();
}

std::vector<RecallMemoryManager::RecallMemoryItem> APIServer::search_embedding(const std::string& user_id, 
                                                                            const std::string& query) {
    // 这里需要实现基于embedding的搜索功能
    return std::vector<RecallMemoryManager::RecallMemoryItem>();
}

std::vector<RecallMemoryManager::RecallMemoryItem> APIServer::search_hybrid(const std::string& user_id,
                                                                         const std::string& query) {
    // 这里需要实现混合搜索功能
    return std::vector<RecallMemoryManager::RecallMemoryItem>();
}

// 核心记忆API
int APIServer::add_core_memory(int user_id, const std::string& key, const std::string& value, 
                              const std::string& description) {
    bool success = coreMemoryManager_->addCoreMemoryBlock(user_id, key, value, description);
    if (success) {
        // 这里需要返回新创建的核心记忆的ID，但方法没有返回ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_core_memory(int memory_id, const std::string& key, const std::string& value, 
                                  const std::string& description) {
    // CoreMemoryBlockManager没有提供更新方法
    return false;
}

bool APIServer::delete_core_memory(int memory_id) {
    // CoreMemoryBlockManager没有提供删除方法
    return false;
}

std::vector<CoreMemoryBlockRecord> APIServer::list_core_memories(int user_id) {
    return coreMemoryManager_->getAllCoreMemoryBlocks(user_id);
}

// 情景记忆API
int APIServer::add_episodic_memory(int user_id, int agent_id, const std::string& event, 
                                  const std::string& content, const std::string& event_type, 
                                  const std::string& path, const std::string& metadata) {
    bool success = EpisodicMemoryManager::addEpisodicMemory(user_id, agent_id, "2023-01-01 00:00:00", 
                                                           event, content, event_type, path, metadata, "./db.sqlite");
    if (success) {
        // 这里需要返回新创建的情景记忆的ID，但静态方法没有返回ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_episodic_memory(int memory_id, const std::string& event, 
                                      const std::string& content, const std::string& event_type, 
                                      const std::string& path, const std::string& metadata) {
    // EpisodicMemoryManager没有提供更新方法
    return false;
}

bool APIServer::delete_episodic_memory(int memory_id) {
    return EpisodicMemoryManager::deleteEpisodicMemory(memory_id, "./db.sqlite");
}

std::vector<EpisodicMemoryRecord> APIServer::list_episodic_memories(int user_id) {
    return EpisodicMemoryManager::getAllEpisodicMemories(user_id, "./db.sqlite");
}

// 语义记忆API
int APIServer::add_semantic_memory(int user_id, int agent_id, const std::string& title, 
                                  const std::string& content, const std::string& description, 
                                  const std::string& path, const std::string& metadata) {
    bool success = SemanticMemoryManager::addSemanticMemory(user_id, agent_id, title, description, content,
                                                          path, metadata, "./db.sqlite");
    if (success) {
        // 这里需要返回新创建的语义记忆的ID，但静态方法没有返回ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_semantic_memory(int memory_id, const std::string& title, 
                                      const std::string& content, const std::string& description, 
                                      const std::string& path, const std::string& metadata) {
    return SemanticMemoryManager::updateSemanticMemory(memory_id, title, description, content,
                                                     metadata, "./db.sqlite");
}

bool APIServer::delete_semantic_memory(int memory_id) {
    return SemanticMemoryManager::deleteSemanticMemory(memory_id, "./db.sqlite");
}

std::vector<SemanticMemoryRecord> APIServer::list_semantic_memories(int user_id) {
    return SemanticMemoryManager::getAllSemanticMemories(user_id, "./db.sqlite");
}

// 程序性记忆API
int APIServer::add_procedural_memory(int user_id, int agent_id, const std::string& title, 
                                    const std::string& description, const std::string& steps, 
                                    const std::string& path, const std::string& metadata) {
    bool success = ProceduralMemoryManager::addProceduralMemory(user_id, agent_id, title, description, steps,
                                                              path, metadata, "./db.sqlite");
    if (success) {
        // 这里需要返回新创建的程序性记忆的ID，但静态方法没有返回ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_procedural_memory(int memory_id, const std::string& title, 
                                        const std::string& description, const std::string& steps, 
                                        const std::string& path, const std::string& metadata) {
    return ProceduralMemoryManager::updateProceduralMemory(memory_id, title, description, steps,
                                                         metadata, "./db.sqlite");
}

bool APIServer::delete_procedural_memory(int memory_id) {
    return ProceduralMemoryManager::deleteProceduralMemory(memory_id, "./db.sqlite");
}

std::vector<ProceduralMemoryRecord> APIServer::list_procedural_memories(int user_id) {
    return ProceduralMemoryManager::getAllProceduralMemories(user_id, "./db.sqlite");
}

// 资源记忆API
int APIServer::add_resource_memory(int user_id, int agent_id, const std::string& title, 
                                  const std::string& content, const std::string& resource_type, 
                                  const std::string& path, const std::string& metadata) {
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = agent_id;
    item.session_id = 1; // 默认值
    item.description = title + ": " + content;  // 合并title和content为description
    item.resource_type = resource_type;
    item.path = path;
    item.metadata = metadata;
    item.size = 0;  // 默认大小为0
    
    bool result = resourceMemoryManager_->add_resource_memory(item);
    // 返回新插入记录的ID，简化处理
    return result ? 1 : -1;
}

bool APIServer::update_resource_memory(int memory_id, const std::string& title, 
                                      const std::string& content, const std::string& resource_type, 
                                      const std::string& path, const std::string& metadata) {
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = 1; // 默认值
    item.agent_id = 1; // 默认值
    item.session_id = 1; // 默认值
    item.description = title + ": " + content;  // 合并title和content为description
    item.resource_type = resource_type;
    item.path = path;
    item.metadata = metadata;
    item.size = 0;  // 默认大小为0
    
    return resourceMemoryManager_->update_resource_memory(memory_id, item);
}

bool APIServer::delete_resource_memory(int memory_id) {
    return resourceMemoryManager_->delete_resource_memory(memory_id);
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> APIServer::list_resource_memories(int user_id) {
    // 需要将ResourceMemoryItem转换为ResourceMemoryRecord
    auto items = resourceMemoryManager_->list_resource_memories(user_id);
    std::vector<ResourceMemoryManager::ResourceMemoryItem> records;
    // 简化处理，实际需要转换
    return items;  // 直接返回items，因为类型已经匹配
}

// 知识库API
int APIServer::add_knowledge_vault_entry(int user_id, int agent_id, const std::string& title, 
                                        const std::string& content, const std::string& category, 
                                        const std::string& path, const std::string& metadata) {
    bool success = knowledgeVaultManager_->addKnowledgeVaultEntry(user_id, agent_id, title, category, path,
                                                               "public", content, metadata);
    if (success) {
        // 这里需要返回新创建的知识库条目的ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_knowledge_vault_entry(int entry_id, const std::string& title, 
                                            const std::string& content, const std::string& category, 
                                            const std::string& path, const std::string& metadata) {
    return knowledgeVaultManager_->updateKnowledgeVaultEntry(entry_id, title, category, path,
                                                           "public", content, metadata);
}

bool APIServer::delete_knowledge_vault_entry(int entry_id) {
    return knowledgeVaultManager_->deleteKnowledgeVaultEntry(entry_id);
}

std::vector<KnowledgeVaultRecord> APIServer::list_knowledge_vault_entries(int user_id) {
    return knowledgeVaultManager_->getAllKnowledgeVaultEntries(user_id);
}

// 归档记忆API
int APIServer::add_archival_memory(int user_id, int agent_id, const std::string& memory_type, 
                                  const std::string& content, const std::string& metadata) {
    bool success = ArchivalMemoryManager::addArchivalMemory(user_id, agent_id, memory_type, content, metadata, "./db.sqlite");
    if (success) {
        // 这里需要返回新创建的归档记忆的ID，但静态方法没有返回ID
        return 1;
    } else {
        return -1; // 表示失败
    }
}

bool APIServer::update_archival_memory(int memory_id, const std::string& memory_type, 
                                      const std::string& content, const std::string& metadata) {
    return ArchivalMemoryManager::updateArchivalMemory(memory_id, content, metadata, "./db.sqlite");
}

bool APIServer::delete_archival_memory(int memory_id) {
    return ArchivalMemoryManager::deleteArchivalMemory(memory_id, "./db.sqlite");
}

std::vector<ArchivalMemoryRecord> APIServer::list_archival_memories(int user_id) {
    return ArchivalMemoryManager::getAllArchivalMemories(user_id, "./db.sqlite");
}

// 模型API
std::string APIServer::chat_completion(int user_id, int session_id, int agent_id, const std::string& message) {
    // 这里需要实现聊天完成功能
    return "Not implemented";
}

std::string APIServer::multimodal_completion(int user_id, int session_id, int agent_id, 
                                           const std::string& text, const std::string& image_path) {
    // 这里需要实现多模态完成功能
    return "Not implemented";
}

// 协调搜索API
std::vector<RecallMemoryManager::RecallMemoryItem> APIServer::coordinated_search(const std::string& user_id,
                                                                             const std::string& query) {
    // 这里需要实现协调搜索功能
    return std::vector<RecallMemoryManager::RecallMemoryItem>();
}