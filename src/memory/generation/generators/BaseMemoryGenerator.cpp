#include "BaseMemoryGenerator.h"
#include "../../../models/QwenAPI.h"
#include "../../../memory/messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"
#include "../../../models/PromptTemplates.h"
#include <iostream>

std::string BaseMemoryGenerator::generate(const std::vector<MessageManager::Message>& messages) {
    // 构建Prompt
    std::string prompt = this->buildGenerationPrompt(messages);
    
    // 调用大模型API生成记忆
    QwenAPI qwen;
    QwenChatCompletionRequest request;
    request.model = "qwen-plus";
    
    QwenMessage message;
    message.role = "user";
    message.content = prompt;
    request.messages.push_back(message);
    
    QwenChatCompletionResponse response = qwen.chat_completion(request);
    
    // 返回响应内容
    return response.content;
}

bool BaseMemoryGenerator::processAndSave(const std::vector<MessageManager::Message>& messages,
                                        int user_id, int agent_id, int session_id,
                                        MemoryOrchestrator& orchestrator) {
    std::cout << "[BaseMemoryGenerator] 开始生成记忆..." << std::endl;
    
    // 生成记忆
    std::string jsonResult = generate(messages);
    
    std::cout << "[BaseMemoryGenerator] 模型返回JSON(前500字符): " 
              << jsonResult.substr(0, std::min(500, (int)jsonResult.length())) << std::endl;
    
    // 保存到数据库
    bool success = saveToDatabaseImpl(jsonResult, user_id, agent_id, session_id, orchestrator);
    
    if (success) {
        std::cout << "[BaseMemoryGenerator] ✓ 记忆保存成功" << std::endl;
    } else {
        std::cout << "[BaseMemoryGenerator] ✗ 记忆保存失败" << std::endl;
    }
    
    return success;
}