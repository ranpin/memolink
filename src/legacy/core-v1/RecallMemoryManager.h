#ifndef RECALL_MEMORY_MANAGER_H
#define RECALL_MEMORY_MANAGER_H

#include "infra/storage/StorageManager.h"
#include <string>
#include <vector>

class RecallMemoryManager {
public:
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

    RecallMemoryManager(StorageManager& storage) : storage_(storage) {}

    bool add_recall_memory(const RecallMemoryItem& item);
    std::vector<RecallMemoryItem> list_recall_memories(int user_id, int limit = 0);
    std::vector<RecallMemoryItem> search_recall_memories(int user_id, const std::string& query);
    
    // 新增方法：查询用户最近几次会话的历史记录
    std::vector<RecallMemoryItem> list_recent_sessions_memories(int user_id, int session_limit = 5, int message_limit = 0);
    
    // 嵌入向量相关方法
    bool saveEmbedding(int memoryId, const std::vector<float>& embedding);
    std::vector<float> getEmbedding(int memoryId);
    std::vector<RecallMemoryItem> searchByEmbedding(int user_id, const std::vector<float>& queryEmbedding, int topK);

private:
    StorageManager& storage_;
};

#endif // RECALL_MEMORY_MANAGER_H