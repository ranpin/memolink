#ifndef RESOURCE_MEMORY_MANAGER_H
#define RESOURCE_MEMORY_MANAGER_H

#include "../core/StorageManager.h"
#include <string>
#include <vector>

struct ResourceMemoryRecord {
    int id;
    int user_id;
    int agent_id;
    std::string filename;
    std::string resource_type;
    std::string summary;
    std::string content;
    int size;
    std::string tree_path;
    std::string metadata;
    std::string last_accessed;
    std::string created_at;
    std::string updated_at;
};

class ResourceMemoryManager {
private:
    StorageManager& storage_;

public:
    explicit ResourceMemoryManager(StorageManager& storage) : storage_(storage) {}
    
    struct ResourceMemoryItem {
        int id;
        int user_id;
        int agent_id;
        std::string title;
        std::string content;
        std::string resource_type;
        std::string path;
        std::string metadata;
        std::string embedding;
        std::string created_at;
    };
    
    bool add_resource_memory(const ResourceMemoryItem& item);
    bool update_resource_memory(int id, const ResourceMemoryItem& item);
    bool delete_resource_memory(int id);
    std::vector<ResourceMemoryItem> list_resource_memories(int user_id, int limit = 100);
    std::vector<ResourceMemoryItem> search_resource_memories(int user_id, const std::string& query);
    
    // 添加处理embedding的方法
    bool saveEmbedding(int resourceId, const std::vector<float>& embedding);
    std::vector<float> getEmbedding(int resourceId);
    std::vector<ResourceMemoryItem> searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK = 10);
};

#endif // RESOURCE_MEMORY_MANAGER_H