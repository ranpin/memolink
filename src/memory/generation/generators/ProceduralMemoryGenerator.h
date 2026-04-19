#ifndef PROCEDURAL_MEMORY_GENERATOR_H
#define PROCEDURAL_MEMORY_GENERATOR_H

#include "BaseMemoryGenerator.h"
#include <string>
#include <vector>
#include "../../messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"

class ProceduralMemoryGenerator : public BaseMemoryGenerator {
public:
    struct GeneratedProceduralMemory {
        std::string skill_name;
        std::string description;
        std::string category;
        std::string steps;
        std::vector<std::string> parameters;
        std::vector<std::string> prerequisites;
        std::string estimated_time;
        float importance = 0.0f;
        std::vector<std::string> tags;
    };

private:
    // 实现基类的纯虚函数
    std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) override;
    bool saveToDatabaseImpl(const std::string& jsonResult,
                           int user_id, int agent_id, int session_id,
                           MemoryOrchestrator& storage) override;
                           
    GeneratedProceduralMemory parseGeneratedMemory(const std::string& jsonResult);
    
public:
    using BaseMemoryGenerator::generate;
    using BaseMemoryGenerator::processAndSave;
};

#endif // PROCEDURAL_MEMORY_GENERATOR_H