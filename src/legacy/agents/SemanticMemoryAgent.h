#pragma once

// SemanticMemoryAgent - 语义记忆代理，管理用户的语义记忆（如事实、概念等）

#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/SemanticMemoryManager.h"
#include <string>
#include <vector>

struct SemanticMemoryItem {
    int id;
    int user_id;
    std::string concept;
    std::string description;
    std::string category;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class SemanticMemoryAgent {
private:
    StorageManager& storage_;
    SemanticMemoryManager semantic_memory_manager_;

public:
    // 构造函数
    SemanticMemoryAgent(StorageManager& storage);
    
    // 语义记忆操作
    int add_semantic_memory(int user_id, const std::string& concept, 
                           const std::string& description, const std::string& category = "",
                           const std::string& metadata = "");
    bool update_semantic_memory(int memory_id, const std::string& concept, 
                               const std::string& description, const std::string& category = "",
                               const std::string& metadata = "");
    bool delete_semantic_memory(int memory_id);
    std::vector<SemanticMemoryItem> list_semantic_memories(int user_id, int limit = 50);
    
    // 语义记忆检索
    std::vector<SemanticMemoryItem> search_semantic_memories(int user_id, const std::string& query);
    
    // 语义记忆摘要
    std::string get_semantic_memory_summary(int user_id, int limit = 10);
};