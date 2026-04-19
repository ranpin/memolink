#include "CoreMemoryGenerator.h"
#include "../../../models/PromptTemplates.h"
#include "../../../models/QwenAPI.h"
#include "../../../database/managers/CoreMemoryBlockManager.h"
#include <json/json.h>
#include <iostream>
#include <sstream>

// 实现基类的纯虚函数
std::string CoreMemoryGenerator::buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) {
    return PromptTemplates::getCoreMemoryGenerationPrompt(messages);
}

bool CoreMemoryGenerator::saveToDatabaseImpl(const std::string& jsonResult,
                                          int user_id, int agent_id, int session_id,
                                          MemoryOrchestrator& orchestrator) {
    try {
        auto parsedMemory = parseGeneratedMemory(jsonResult);
        
        // 使用MemoryOrchestrator保存记忆
        auto& coreManager = orchestrator.getCoreMemoryBlockManager();
        
        CoreMemory memory;
        memory.user_id = user_id;
        memory.agent_id = agent_id;
        memory.memory_type = "core";
        memory.content = parsedMemory.preference_details;
        memory.priority = static_cast<int>(parsedMemory.importance * 100); // 转换为0-100的重要性值
        memory.metadata = "{}";
        
        int64_t result = coreManager.add(memory);
        return result > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error saving core memory: " << e.what() << std::endl;
        return false;
    }
}


CoreMemoryGenerator::GeneratedCoreMemory CoreMemoryGenerator::parseGeneratedMemory(const std::string& jsonResult) {
    GeneratedCoreMemory memory;
    
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(jsonResult, root)) {
        memory.preference_summary = root.get("preference_summary", "").asString();
        memory.preference_details = root.get("preference_details", "").asString();
        memory.user_info = root.get("user_info", "").asString();
        
        // 解析重要事实
        const Json::Value& importantFacts = root["important_facts"];
        if (importantFacts.isArray()) {
            for (const auto& fact : importantFacts) {
                memory.important_facts.push_back(fact.asString());
            }
        }
        
        memory.importance = root.get("importance", 0.0f).asFloat();
        
        // 解析标签
        const Json::Value& tags = root["tags"];
        if (tags.isArray()) {
            for (const auto& tag : tags) {
                memory.tags.push_back(tag.asString());
            }
        }
    }
    
    return memory;
}