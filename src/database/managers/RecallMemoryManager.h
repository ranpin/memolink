#ifndef RECALL_MEMORY_MANAGER_H
#define RECALL_MEMORY_MANAGER_H

#include "../core/StorageManager.h"
#include <string>
#include <vector>

class RecallMemoryManager {
private:
    StorageManager& storage_;

public:
    explicit RecallMemoryManager(StorageManager& storage) : storage_(storage) {}
    
    struct RecallMemoryItem {
        int id;
        int user_id;
        int agent_id;
        int session_id;
        std::string message_type;
        std::string content;
        std::string metadata;
        std::string embedding;
        std::string created_at;
        std::string updated_at;
    };
    
    bool add_recall_memory(const RecallMemoryItem& item);
    bool update_recall_memory(int id, const RecallMemoryItem& item);
    bool delete_recall_memory(int id);
    std::vector<RecallMemoryItem> list_recall_memories(int user_id, int limit = 100);
    std::vector<RecallMemoryItem> search_recall_memories(int user_id, const std::string& query);
    
    // 添加处理embedding的方法
    bool saveEmbedding(int memoryId, const std::vector<float>& embedding);
    std::vector<float> getEmbedding(int memoryId);
    std::vector<RecallMemoryItem> searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK = 10);
};

#endif