#ifndef CORE_MEMORY_GENERATOR_H
#define CORE_MEMORY_GENERATOR_H

#include "BaseMemoryGenerator.h"
#include <string>
#include <vector>
#include "../../messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"

class CoreMemoryGenerator : public BaseMemoryGenerator {
public:
    struct GeneratedCoreMemory {
        std::string preference_summary;
        std::string preference_details;
        std::string user_info;
        std::vector<std::string> important_facts;
        std::vector<std::string> tags;
        std::vector<std::string> categories;
        float importance = 0.0f;
    };

private:
    // 实现基类的纯虚函数
    std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) override;
    bool saveToDatabaseImpl(const std::string& jsonResult,
                           int user_id, int agent_id, int session_id,
                           MemoryOrchestrator& storage) override;
                           
    GeneratedCoreMemory parseGeneratedMemory(const std::string& jsonResult);
    
public:
    using BaseMemoryGenerator::generate;
    using BaseMemoryGenerator::processAndSave;
};

#endif // CORE_MEMORY_GENERATOR_H