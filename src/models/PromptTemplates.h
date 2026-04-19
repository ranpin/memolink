#ifndef PROMPT_TEMPLATES_H
#define PROMPT_TEMPLATES_H

#include <string>
#include <vector>
#include "../memory/messages/MessageManager.h"

class PromptTemplates {
public:
    // 记忆分类Prompt模板
    static std::string getMemoryClassificationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 事件记忆生成Prompt模板
    static std::string getEpisodicMemoryGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 语义记忆生成Prompt模板
    static std::string getSemanticMemoryGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 核心记忆生成Prompt模板
    static std::string getCoreMemoryGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 程序记忆生成Prompt模板
    static std::string getProceduralMemoryGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 资源记忆生成Prompt模板
    static std::string getResourceMemoryGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 私密信息库记忆生成Prompt模板
    static std::string getSecretVaultGenerationPrompt(
        const std::vector<MessageManager::Message>& messages);
    
    // 图片描述结构化Prompt模板
    static std::string getImageDescriptionPrompt();
};

#endif // PROMPT_TEMPLATES_H