#ifndef EPISODIC_MEMORY_GENERATOR_H
#define EPISODIC_MEMORY_GENERATOR_H

#include "BaseMemoryGenerator.h"
#include <string>
#include <vector>
#include "../../messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"

class EpisodicMemoryGenerator : public BaseMemoryGenerator {
public:
    struct GeneratedEpisodicMemory {
        std::string event_summary;
        std::string event_details;
        std::string event_type;
        std::vector<std::string> participants;
        std::vector<std::string> key_points;
        std::string emotional_tone;
        float importance = 0.0f;
        std::vector<std::string> tags;
    };

private:
    // 实现基类的纯虚函数
    std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) override;
    bool saveToDatabaseImpl(const std::string& jsonResult,
                           int user_id, int agent_id, int session_id,
                           MemoryOrchestrator& storage) override;
                           
    GeneratedEpisodicMemory parseGeneratedMemory(const std::string& jsonResult);
    
public:
    using BaseMemoryGenerator::generate;
    using BaseMemoryGenerator::processAndSave;
};

#endif // EPISODIC_MEMORY_GENERATOR_H