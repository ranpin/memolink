#include <vector>
#include <string>
#include "legacy/agents/CoreMemoryAgent.h"
#include <iostream>

CoreMemoryAgent::CoreMemoryAgent(StorageManager& storage) 
    : storage_(storage), core_memory_manager_(storage) {
}

bool CoreMemoryAgent::set_user_core_memory(int user_id, const std::string& key, const std::string& value) {
    return core_memory_manager_.addCoreMemoryBlock(user_id, key, value, "");
}

std::string CoreMemoryAgent::get_user_core_memory(int user_id, const std::string& key) {
    auto items = core_memory_manager_.listCoreMemoryBlocks(user_id);
    for (const auto& item : items) {
        if (item.first == key) {
            return item.second;
        }
    }
    return ""; // 未找到
}

bool CoreMemoryAgent::delete_user_core_memory(int user_id, const std::string& key) {
    // CoreMemoryBlockManager没有直接的删除方法，需要实现
    // 通过添加一个空值来"删除"核心记忆
    bool result = core_memory_manager_.addCoreMemoryBlock(user_id, key, "", "");
    std::cout << "Deleting core memory for user " << user_id << ", key " << key << std::endl;
    return result;
}

std::vector<CoreMemoryBlockItem> CoreMemoryAgent::list_user_core_memories(int user_id) {
    std::vector<CoreMemoryBlockItem> result;
    auto items = core_memory_manager_.listCoreMemoryBlocks(user_id);
    
    for (const auto& item : items) {
        CoreMemoryBlockItem memory_item;
        memory_item.user_id = user_id;
        memory_item.key = item.first;
        memory_item.value = item.second;
        result.push_back(memory_item);
    }
    
    return result;
}

bool CoreMemoryAgent::update_user_core_memory(int user_id, const std::string& key, const std::string& value) {
    // 通过重新添加来更新核心记忆
    bool result = core_memory_manager_.addCoreMemoryBlock(user_id, key, value, "");
    std::cout << "Updating core memory for user " << user_id << ", key " << key << std::endl;
    return result;
}

std::string CoreMemoryAgent::get_core_memory_summary(int user_id) {
    auto items = list_user_core_memories(user_id);
    std::string summary = "Core memory summary for user " + std::to_string(user_id) + ":\n";
    
    for (const auto& item : items) {
        summary += "- " + item.key + ": " + item.value + "\n";
    }
    
    return summary;
}

std::vector<CoreMemoryBlockItem> CoreMemoryAgent::getAllCoreMemoryBlocks(int user_id) {
    std::vector<CoreMemoryBlockItem> result;
    auto records = core_memory_manager_.getAllCoreMemoryBlocks(user_id);
    
    for (const auto& record : records) {
        CoreMemoryBlockItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.key = record.label;
        item.value = record.value;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    return result;
}

CoreMemoryBlockItem CoreMemoryAgent::getCoreMemoryBlockById(int id) {
    CoreMemoryBlockItem item;
    auto record = core_memory_manager_.getCoreMemoryBlockById(id);
    
    item.id = record.id;
    item.user_id = record.user_id;
    item.key = record.label;
    item.value = record.value;
    item.created_at = "";
    item.updated_at = "";
    
    return item;
}

std::vector<CoreMemoryBlockItem> CoreMemoryAgent::searchCoreMemoryBlocks(int user_id, const std::string& keyword) {
    std::vector<CoreMemoryBlockItem> result;
    auto records = core_memory_manager_.searchCoreMemoryBlocks(user_id, keyword);
    
    for (const auto& record : records) {
        CoreMemoryBlockItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.key = record.label;
        item.value = record.value;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    return result;
}