#include "ResourceMemoryAgent.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../utils/TimeUtils.h"
#include <iostream>

ResourceMemoryAgent::ResourceMemoryAgent(StorageManager& storage) 
    : storage_(storage), resource_memory_manager_(storage) {
}

void ResourceMemoryAgent::initialize(int userId, int agentId) {
    user_id_ = userId;
    agent_id_ = agentId;
}

bool ResourceMemoryAgent::addResourceMemory(const std::string& title, const std::string& content, 
                                           const std::string& resourceType, const std::string& path, 
                                           const std::string& metadata) {
    ResourceMemoryManager manager(storage_);
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id_;
    item.agent_id = agent_id_;
    item.title = title;
    item.content = content;
    item.resource_type = resourceType;
    item.path = path;
    item.metadata = metadata;
    return manager.add_resource_memory(item);
}

bool ResourceMemoryAgent::updateResourceMemory(int memoryId, const std::string& title, const std::string& content, 
                                              const std::string& resourceType, const std::string& path, 
                                              const std::string& metadata) {
    ResourceMemoryManager manager(storage_);
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id_;
    item.agent_id = agent_id_;
    item.title = title;
    item.content = content;
    item.resource_type = resourceType;
    item.path = path;
    item.metadata = metadata;
    return manager.update_resource_memory(memoryId, item);
}

bool ResourceMemoryAgent::deleteResourceMemory(int memoryId) {
    ResourceMemoryManager manager(storage_);
    return manager.delete_resource_memory(memoryId);
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryAgent::listResourceMemories() {
    ResourceMemoryManager manager(storage_);
    return manager.list_resource_memories(user_id_);
}

std::vector<ResourceMemoryManager::ResourceMemoryItem> ResourceMemoryAgent::searchResourceMemories(const std::string& query) {
    ResourceMemoryManager manager(storage_);
    return manager.search_resource_memories(user_id_, query);
}

std::string ResourceMemoryAgent::get_resource_memory_summary(int limit) {
    auto memories = listResourceMemories();
    if (limit > 0 && static_cast<size_t>(limit) < memories.size()) {
        memories.erase(memories.begin(), memories.end() - limit);
    }
    
    std::string summary = "Resource memory summary for user " + std::to_string(user_id_) + ":\n";
    for (const auto& memory : memories) {
        summary += "- " + memory.title + ": " + memory.content + "\n";
    }
    return summary;
}