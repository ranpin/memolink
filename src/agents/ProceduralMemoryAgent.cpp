#include <string>
#include "ProceduralMemoryAgent.h"
#include "../database/managers/ProceduralMemoryManager.h"
#include <iostream>
#include <vector>

ProceduralMemoryAgent::ProceduralMemoryAgent(StorageManager& storage) 
    : storage_(storage), procedural_memory_manager_() {
}

int ProceduralMemoryAgent::add_procedural_memory(int user_id, const std::string& skill_name,
                                               const std::string& description, const std::string& steps,
                                               const std::string& category, int proficiency_level,
                                               const std::string& metadata) {
    // 由于ProceduralMemoryManager只有静态方法，我们直接调用它们
    bool result = ProceduralMemoryManager::addProceduralMemory(
        user_id, 1, skill_name, description, steps, category, metadata, storage_.getDatabasePath());
    
    if (result) {
        std::cout << "Added procedural memory for user " << user_id << std::endl;
        return 1; // 临时返回值
    }
    return -1;
}

bool ProceduralMemoryAgent::update_procedural_memory(int memory_id, const std::string& skill_name,
                                                   const std::string& description, const std::string& steps,
                                                   const std::string& category, int proficiency_level,
                                                   const std::string& metadata) {
    bool result = ProceduralMemoryManager::updateProceduralMemory(
        memory_id, skill_name, description, steps, metadata, storage_.getDatabasePath());
    
    std::cout << "Updated procedural memory " << memory_id << std::endl;
    return result;
}

bool ProceduralMemoryAgent::delete_procedural_memory(int memory_id) {
    bool result = ProceduralMemoryManager::deleteProceduralMemory(memory_id, storage_.getDatabasePath());
    
    std::cout << "Deleted procedural memory " << memory_id << std::endl;
    return result;
}

std::vector<ProceduralMemoryItem> ProceduralMemoryAgent::list_procedural_memories(int user_id, int limit) {
    auto records = ProceduralMemoryManager::getAllProceduralMemories(user_id, storage_.getDatabasePath());
    
    std::vector<ProceduralMemoryItem> result;
    int count = 0;
    for (const auto& record : records) {
        if (limit > 0 && count >= limit) break;
        
        ProceduralMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.skill_name = record.title;
        item.description = record.summary;
        item.steps = record.steps;
        item.category = "";
        item.proficiency_level = 1;
        item.last_practiced = "";
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
        count++;
    }
    
    std::cout << "Listing procedural memories for user " << user_id << std::endl;
    return result;
}

std::vector<ProceduralMemoryItem> ProceduralMemoryAgent::search_procedural_memories(int user_id, const std::string& query) {
    auto records = ProceduralMemoryManager::searchProceduralMemories(user_id, query, storage_.getDatabasePath());
    
    std::vector<ProceduralMemoryItem> result;
    for (const auto& record : records) {
        ProceduralMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.skill_name = record.title;
        item.description = record.summary;
        item.steps = record.steps;
        item.category = "";
        item.proficiency_level = 1;
        item.last_practiced = "";
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    std::cout << "Searching procedural memories for user " << user_id << " with query " << query << std::endl;
    return result;
}

std::string ProceduralMemoryAgent::get_procedural_memory_summary(int user_id, int limit) {
    auto memories = list_procedural_memories(user_id, limit);
    std::string summary = "Procedural memory summary for user " + std::to_string(user_id) + ":\n";
    
    for (const auto& memory : memories) {
        summary += "- " + memory.skill_name + ": " + memory.description + "\n";
    }
    
    return summary;
}