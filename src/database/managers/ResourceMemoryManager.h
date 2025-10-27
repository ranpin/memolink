#ifndef RESOURCE_MEMORY_MANAGER_H
#define RESOURCE_MEMORY_MANAGER_H

#include "../core/StorageManager.h"
#include <string>
#include <vector>

class ResourceMemoryManager {
private:
    StorageManager& storage_;

public:
    explicit ResourceMemoryManager(StorageManager& storage) : storage_(storage) {}
    
    struct ResourceMemoryItem {
        int id;
        int user_id;
        int agent_id;
        int session_id;
        std::string path;
        std::string description;
        std::string resource_type;
        int size;
        std::string metadata;
        std::string embedding;
        std::string created_at;
        std::string updated_at;
    };
    
    bool add_resource_memory(const ResourceMemoryItem& item);
    bool update_resource_memory(int id, const ResourceMemoryItem& item);
    bool delete_resource_memory(int id);
    ResourceMemoryItem get_last_resource_memory(int user_id);
    std::vector<ResourceMemoryItem> list_resource_memories(int user_id, int limit = 10);
    std::vector<ResourceMemoryItem> search_resource_memories(int user_id, const std::string& query, int limit = 5);
    ResourceMemoryItem get_resource_memory(int id);
    bool saveEmbedding(int memory_id, const std::vector<float>& embedding);
    std::vector<float> getEmbedding(int resourceId);
    std::vector<ResourceMemoryItem> searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK = 5);
};

#endif // RESOURCE_MEMORY_MANAGER_H