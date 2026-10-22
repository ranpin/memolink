#pragma once

// ProceduralMemoryAgent - 程序记忆代理，管理用户的程序记忆（如技能、习惯等）

#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/ProceduralMemoryManager.h"
#include <string>
#include <vector>

struct ProceduralMemoryItem {
    int id;
    int user_id;
    std::string skill_name;
    std::string description;
    std::string steps;
    std::string category;
    int proficiency_level;
    std::string last_practiced;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class ProceduralMemoryAgent {
private:
    StorageManager& storage_;
    ProceduralMemoryManager procedural_memory_manager_;

public:
    // 构造函数
    ProceduralMemoryAgent(StorageManager& storage);
    
    // 程序记忆操作
    int add_procedural_memory(int user_id, const std::string& skill_name,
                             const std::string& description, const std::string& steps,
                             const std::string& category = "", int proficiency_level = 1,
                             const std::string& metadata = "");
    bool update_procedural_memory(int memory_id, const std::string& skill_name,
                                 const std::string& description, const std::string& steps,
                                 const std::string& category = "", int proficiency_level = 1,
                                 const std::string& metadata = "");
    bool delete_procedural_memory(int memory_id);
    std::vector<ProceduralMemoryItem> list_procedural_memories(int user_id, int limit = 50);
    
    // 程序记忆检索
    std::vector<ProceduralMemoryItem> search_procedural_memories(int user_id, const std::string& query);
    
    // 程序记忆摘要
    std::string get_procedural_memory_summary(int user_id, int limit = 10);
};