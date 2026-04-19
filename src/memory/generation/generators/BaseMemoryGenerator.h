#ifndef BASE_MEMORY_GENERATOR_H
#define BASE_MEMORY_GENERATOR_H

#include <string>
#include <vector>
#include "../../../models/QwenAPI.h"
#include "../../../memory/messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"
#include "../../../models/PromptTemplates.h"

class BaseMemoryGenerator {
protected:
    // 子类需要实现的方法
    virtual std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) = 0;
    virtual bool saveToDatabaseImpl(const std::string& jsonResult,
                                   int user_id, int agent_id, int session_id,
                                   MemoryOrchestrator& storage) = 0;

public:
    virtual ~BaseMemoryGenerator() = default;
    
    // 通用的生成逻辑
    std::string generate(const std::vector<MessageManager::Message>& messages);
    
    // 通用的保存逻辑
    bool processAndSave(const std::vector<MessageManager::Message>& messages,
                       int user_id, int agent_id, int session_id,
                       MemoryOrchestrator& storage);
};

#endif // BASE_MEMORY_GENERATOR_H