#pragma once

// EpisodicMemoryAgent - 事件记忆代理，管理用户的事件记忆（如经历、活动等）

#include "../database/core/StorageManager.h"
#include "../database/managers/EpisodicMemoryManager.h"
#include <string>
#include <vector>

struct EpisodicMemoryItem {
    int id;
    int user_id;
    std::string timestamp;
    std::string summary;
    std::string details;
    std::string event_type;
    std::string tree_path;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class EpisodicMemoryAgent {
private:
    StorageManager& storage_;
    EpisodicMemoryManager episodic_memory_manager_;

public:
    // 构造函数
    EpisodicMemoryAgent(StorageManager& storage);
    
    // 事件记忆操作
    int add_episodic_memory(int user_id, const std::string& content, const std::string& metadata = "");
    bool update_episodic_memory(int memory_id, const std::string& content, const std::string& metadata = "");
    bool delete_episodic_memory(int memory_id);
    std::vector<EpisodicMemoryItem> list_episodic_memories(int user_id, int limit = 50);
    
    // 事件记忆检索
    std::vector<EpisodicMemoryItem> search_episodic_memories(int user_id, const std::string& query);
    
    // 事件记忆摘要
    std::string get_episodic_memory_summary(int user_id, int limit = 10);
};