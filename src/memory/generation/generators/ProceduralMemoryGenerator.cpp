#include "ProceduralMemoryGenerator.h"
#include "../../../models/PromptTemplates.h"
#include "../../../models/QwenAPI.h"
#include "../../../database/managers/ProceduralMemoryManager.h"  // 修复路径
#include <json/json.h>
#include <iostream>

// 实现基类的纯虚函数
std::string ProceduralMemoryGenerator::buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) {
    return PromptTemplates::getProceduralMemoryGenerationPrompt(messages);
}

bool ProceduralMemoryGenerator::saveToDatabaseImpl(const std::string& jsonResult,
                                                int user_id, int agent_id, int session_id,
                                                MemoryOrchestrator& orchestrator) {
    try {
        auto parsedMemory = parseGeneratedMemory(jsonResult);
        
        // 使用MemoryOrchestrator保存记忆
        auto& proceduralManager = orchestrator.getProceduralMemoryManager();  // 修复方法名
        
        ProceduralMemory memory;
        memory.user_id = user_id;
        memory.agent_id = agent_id;
        memory.name = parsedMemory.skill_name;  // 使用正确的字段名
        memory.description = parsedMemory.description;  // 使用正确的字段名
        memory.steps = parsedMemory.steps;  // 使用正确的字段名
        memory.metadata = "{}";
        
        int64_t result = proceduralManager.add(memory);
        return result > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error saving procedural memory: " << e.what() << std::endl;
        return false;
    }
}

ProceduralMemoryGenerator::GeneratedProceduralMemory ProceduralMemoryGenerator::parseGeneratedMemory(const std::string& jsonResult) {
    GeneratedProceduralMemory memory;
    
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(jsonResult, root)) {
        memory.skill_name = root.get("skill_name", "").asString();
        memory.description = root.get("description", "").asString();
        memory.category = root.get("category", "").asString();
        
        // 解析步骤
        const Json::Value& steps = root["steps"];
        if (steps.isArray()) {
            for (const auto& step : steps) {
                memory.steps += step.asString() + "\n";
            }
        }
        
        memory.importance = root.get("importance", 0.0f).asFloat();
    }
    
    return memory;
}