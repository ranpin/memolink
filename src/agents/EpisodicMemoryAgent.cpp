#include "EpisodicMemoryAgent.h"
#include "../database/managers/EpisodicMemoryManager.h"
#include "../utils/TimeUtils.h"
#include <iostream>
#include <vector>

EpisodicMemoryAgent::EpisodicMemoryAgent(StorageManager& storage) 
    : storage_(storage), episodic_memory_manager_() {
}

int EpisodicMemoryAgent::add_episodic_memory(int user_id, const std::string& content, const std::string& metadata) {
    // 获取当前北京时间戳
    std::string timestamp = TimeUtils::getCurrentTimestamp();
    
    // 添加事件记忆
    bool result = EpisodicMemoryManager::addEpisodicMemory(
        user_id, 1, timestamp, content, content, "event", "", metadata, storage_.getDatabasePath());
    
    if (result) {
        std::cout << "Added episodic memory for user " << user_id << std::endl;
        return 1; // 临时返回值
    }
    return -1;
}

bool EpisodicMemoryAgent::update_episodic_memory(int memory_id, const std::string& content, const std::string& metadata) {
    bool result = EpisodicMemoryManager::updateEpisodicMemory(
        memory_id, content, content, metadata, storage_.getDatabasePath());
    
    std::cout << "Updated episodic memory " << memory_id << std::endl;
    return result;
}

bool EpisodicMemoryAgent::delete_episodic_memory(int memory_id) {
    bool result = EpisodicMemoryManager::deleteEpisodicMemory(memory_id, storage_.getDatabasePath());
    
    std::cout << "Deleted episodic memory " << memory_id << std::endl;
    return result;
}

std::vector<EpisodicMemoryItem> EpisodicMemoryAgent::list_episodic_memories(int user_id, int limit) {
    auto records = EpisodicMemoryManager::getAllEpisodicMemories(user_id, storage_.getDatabasePath());
    
    std::vector<EpisodicMemoryItem> result;
    for (const auto& record : records) {
        EpisodicMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.timestamp = record.timestamp;
        item.summary = record.event;
        item.details = record.details;
        item.event_type = record.event_type;
        item.tree_path = record.tree_path;
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    // 如果指定了限制数量，只返回最新的limit条记录
    if (limit > 0 && static_cast<size_t>(limit) < result.size()) {
        result.erase(result.begin(), result.end() - limit);
    }
    
    std::cout << "Listing episodic memories for user " << user_id << std::endl;
    return result;
}

std::vector<EpisodicMemoryItem> EpisodicMemoryAgent::search_episodic_memories(int user_id, const std::string& query) {
    auto records = EpisodicMemoryManager::searchEpisodicMemories(user_id, query, storage_.getDatabasePath());
    
    std::vector<EpisodicMemoryItem> result;
    for (const auto& record : records) {
        EpisodicMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.timestamp = record.timestamp;
        item.summary = record.event;
        item.details = record.details;
        item.event_type = record.event_type;
        item.tree_path = record.tree_path;
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    std::cout << "Searching episodic memories for user " << user_id << " with query " << query << std::endl;
    return result;
}

std::string EpisodicMemoryAgent::get_episodic_memory_summary(int user_id, int limit) {
    auto memories = list_episodic_memories(user_id, limit);
    std::string summary = "Episodic memory summary for user " + std::to_string(user_id) + ":\n";
    
    for (const auto& memory : memories) {
        summary += "- " + memory.timestamp + ": " + memory.summary + "\n";
    }
    
    return summary;
}