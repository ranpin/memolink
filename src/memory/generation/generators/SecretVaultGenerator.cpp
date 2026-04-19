#include "SecretVaultGenerator.h"
#include "../../../models/PromptTemplates.h"
#include "../../../models/QwenAPI.h"
#include "../../../database/managers/SecretVaultManager.h"
#include <json/json.h>
#include <iostream>
#include <sstream>

std::string SecretVaultGenerator::buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) {
    return PromptTemplates::getSecretVaultGenerationPrompt(messages);
}

bool SecretVaultGenerator::saveToDatabaseImpl(const std::string& jsonResult,
                                           int user_id, int agent_id, int session_id,
                                           MemoryOrchestrator& orchestrator) {
    try {
        auto parsedMemory = parseGeneratedMemory(jsonResult);
        
        // 使用MemoryOrchestrator保存记忆
        auto& secretManager = orchestrator.getSecretVaultManager();
        
        SecretVault memory;
        memory.user_id = user_id;
        memory.agent_id = agent_id;
        memory.category = parsedMemory.secret_type;
        memory.title = parsedMemory.title;
        memory.content = parsedMemory.content;
        memory.metadata = "{}";
        
        int64_t result = secretManager.add(memory);
        return result > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error saving secret vault memory: " << e.what() << std::endl;
        return false;
    }
}

SecretVaultGenerator::GeneratedSecretVault SecretVaultGenerator::parseGeneratedMemory(const std::string& jsonResult) {
    GeneratedSecretVault memory;
    
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(jsonResult, root)) {
        memory.secret_type = root.get("secret_type", "").asString();
        memory.content = root.get("content", "").asString();
        memory.access_level = root.get("access_level", "").asString();
        
        // 解析标签
        const Json::Value& tags = root["tags"];
        if (tags.isArray()) {
            for (const auto& tag : tags) {
                memory.tags.push_back(tag.asString());
            }
        }
        
        memory.importance = root.get("importance", 0.0f).asFloat();
    }
    
    return memory;
}


