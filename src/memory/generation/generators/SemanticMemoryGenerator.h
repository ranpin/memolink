#ifndef SEMANTIC_MEMORY_GENERATOR_H
#define SEMANTIC_MEMORY_GENERATOR_H

#include "BaseMemoryGenerator.h"
#include <string>
#include <vector>
#include "../../messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"

class SemanticMemoryGenerator : public BaseMemoryGenerator {
public:
    struct GeneratedSemanticMemory {
        std::string concept_name;
        std::string fact_details;
        std::string category;
        std::vector<std::string> related_concepts;
        std::vector<std::string> sources;
        std::vector<std::string> tags;
        float importance = 0.0f;
        std::string confidence_level;
    };

private:
    // 实现基类的纯虚函数
    std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) override;
    bool saveToDatabaseImpl(const std::string& jsonResult,
                           int user_id, int agent_id, int session_id,
                           MemoryOrchestrator& storage) override;
                           
    GeneratedSemanticMemory parseGeneratedMemory(const std::string& jsonResult);
    
public:
    using BaseMemoryGenerator::generate;
    using BaseMemoryGenerator::processAndSave;
};

#endif // SEMANTIC_MEMORY_GENERATOR_H