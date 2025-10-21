#pragma once

// ResourceMemoryAgent - 资源记忆代理，管理用户的资源记忆（如文件、链接等）

#include "../database/core/StorageManager.h"
#include "../database/managers/ResourceMemoryManager.h"
#include <string>
#include <vector>

struct ResourceMemoryItem {
    int id;
    int user_id;
    std::string resource_type;
    std::string title;
    std::string url;
    std::string file_path;
    std::string description;
    std::string tags;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class ResourceMemoryAgent {
private:
    StorageManager& storage_;
    ResourceMemoryManager resource_memory_manager_;
    int user_id_;
    int agent_id_;

public:
    // 构造函数
    ResourceMemoryAgent(StorageManager& storage);
    
    void initialize(int userId, int agentId);
    
    // 资源记忆操作
    bool addResourceMemory(const std::string& title, const std::string& content, 
                          const std::string& resourceType, const std::string& path, 
                          const std::string& metadata);
    bool updateResourceMemory(int memoryId, const std::string& title, const std::string& content, 
                             const std::string& resourceType, const std::string& path, 
                             const std::string& metadata);
    bool deleteResourceMemory(int memoryId);
    std::vector<ResourceMemoryManager::ResourceMemoryItem> listResourceMemories();
    std::vector<ResourceMemoryManager::ResourceMemoryItem> searchResourceMemories(const std::string& query);
    std::string get_resource_memory_summary(int limit = 10);
};