#include <string>
#include "SemanticMemoryAgent.h"
#include "../database/managers/SemanticMemoryManager.h"
#include <iostream>
#include <vector>

SemanticMemoryAgent::SemanticMemoryAgent(StorageManager& storage) 
    : storage_(storage), semantic_memory_manager_() {
}

int SemanticMemoryAgent::add_semantic_memory(int user_id, const std::string& concept, 
                                           const std::string& description, const std::string& category,
                                           const std::string& metadata) {
    // 由于SemanticMemoryManager只有静态方法，我们直接调用它们
    bool result = SemanticMemoryManager::addSemanticMemory(
        user_id, 1, concept, concept, description, "", metadata, storage_.getDatabasePath());
    
    if (result) {
        std::cout << "Added semantic memory for user " << user_id << std::endl;
        return 1; // 临时返回值
    }
    return -1;
}

bool SemanticMemoryAgent::update_semantic_memory(int memory_id, const std::string& concept, 
                                               const std::string& description, const std::string& category,
                                               const std::string& metadata) {
    bool result = SemanticMemoryManager::updateSemanticMemory(
        memory_id, concept, concept, description, metadata, storage_.getDatabasePath());
    
    std::cout << "Updated semantic memory " << memory_id << std::endl;
    return result;
}

bool SemanticMemoryAgent::delete_semantic_memory(int memory_id) {
    bool result = SemanticMemoryManager::deleteSemanticMemory(memory_id, storage_.getDatabasePath());
    
    std::cout << "Deleted semantic memory " << memory_id << std::endl;
    return result;
}

std::vector<SemanticMemoryItem> SemanticMemoryAgent::list_semantic_memories(int user_id, int limit) {
    auto records = SemanticMemoryManager::getAllSemanticMemories(user_id, storage_.getDatabasePath());
    
    std::vector<SemanticMemoryItem> result;
    int count = 0;
    for (const auto& record : records) {
        if (limit > 0 && count >= limit) break;
        
        SemanticMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.concept = record.name;
        item.description = record.details;
        item.category = "";
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
        count++;
    }
    
    std::cout << "Listing semantic memories for user " << user_id << std::endl;
    return result;
}

std::vector<SemanticMemoryItem> SemanticMemoryAgent::search_semantic_memories(int user_id, const std::string& query) {
    auto records = SemanticMemoryManager::searchSemanticMemories(user_id, query, storage_.getDatabasePath());
    
    std::vector<SemanticMemoryItem> result;
    for (const auto& record : records) {
        SemanticMemoryItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.concept = record.name;
        item.description = record.details;
        item.category = "";
        item.metadata = record.metadata;
        item.created_at = "";
        item.updated_at = "";
        result.push_back(item);
    }
    
    std::cout << "Searching semantic memories for user " << user_id << " with query " << query << std::endl;
    return result;
}

std::string SemanticMemoryAgent::get_semantic_memory_summary(int user_id, int limit) {
    auto memories = list_semantic_memories(user_id, limit);
    std::string summary = "Semantic memory summary for user " + std::to_string(user_id) + ":\n";
    
    for (const auto& memory : memories) {
        summary += "- " + memory.concept + ": " + memory.description + "\n";
    }
    
    return summary;
}