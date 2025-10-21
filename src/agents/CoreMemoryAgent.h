#pragma once

// CoreMemoryAgent - 核心记忆代理，管理用户的核心信息（如persona、偏好设置等）

#include "../database/core/StorageManager.h"
#include "../database/managers/CoreMemoryBlockManager.h"
#include <string>
#include <vector>

struct CoreMemoryBlockItem {
    int id;
    int user_id;
    std::string key;
    std::string value;
    std::string created_at;
    std::string updated_at;
};

class CoreMemoryAgent {
private:
    StorageManager& storage_;
    CoreMemoryBlockManager core_memory_manager_;

public:
    // 构造函数
    CoreMemoryAgent(StorageManager& storage);
    
    // 核心记忆操作
    bool set_user_core_memory(int user_id, const std::string& key, const std::string& value);
    std::string get_user_core_memory(int user_id, const std::string& key);
    bool delete_user_core_memory(int user_id, const std::string& key);
    std::vector<CoreMemoryBlockItem> list_user_core_memories(int user_id);
    
    // 核心记忆更新
    bool update_user_core_memory(int user_id, const std::string& key, const std::string& value);
    
    // 核心记忆查询
    std::vector<CoreMemoryBlockItem> getAllCoreMemoryBlocks(int user_id);
    CoreMemoryBlockItem getCoreMemoryBlockById(int id);
    std::vector<CoreMemoryBlockItem> searchCoreMemoryBlocks(int user_id, const std::string& keyword);
    
    // 核心记忆摘要
    std::string get_core_memory_summary(int user_id);
};