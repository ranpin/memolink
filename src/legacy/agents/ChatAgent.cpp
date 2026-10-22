#include <string>
#include "legacy/agents/ChatAgent.h"
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "infra/storage/TimeUtils.h"
#include <iostream>
#include <sstream>

ChatAgent::ChatAgent(StorageManager& storage) 
    : storage_(storage), user_id_(-1), agent_id_(-1) {
}

void ChatAgent::initialize(int userId, int agentId) {
    user_id_ = userId;
    agent_id_ = agentId;
}

std::string ChatAgent::buildContext(int user_id, int agent_id, int session_id, const std::string& message) {
    std::ostringstream context;
    context << "=== 对话历史 ===\n";
    
    // 使用新的RecallMemoryManager接口
    RecallMemoryManager recall_memory_manager(storage_);
    auto recent_memories = recall_memory_manager.list_recall_memories(user_id, 10); // 获取最近10条记录
    
    for (const auto& memory : recent_memories) {
        context << "[" << memory.message_type << "] " << memory.content 
                << " (Created: " << memory.created_at << ")\n";
    }
    
    context << "=== 多模态数据 ===\n";
    
    // 添加多模态数据上下文
    ResourceMemoryManager resource_manager(storage_);
    auto recent_resources = resource_manager.list_resource_memories(user_id, 5); // 获取最近5个多模态资源
    
    for (const auto& resource : recent_resources) {
        context << "[" << resource.resource_type << "] " << resource.path 
                << " (Created: " << resource.created_at << ")\n";
    }
    
    return context.str();
}

std::string ChatAgent::chat(int user_id, int agent_id, int session_id, const std::string& message) {
    // 构建上下文
    std::string context = buildContext(user_id, agent_id, session_id, message);
    
    // 保存用户消息到回忆记忆
    RecallMemoryManager recall_manager(storage_);
    RecallMemoryManager::RecallMemoryItem user_item;
    user_item.user_id = user_id;
    user_item.agent_id = agent_id;
    user_item.session_id = session_id;
    user_item.message_type = "user_message";
    user_item.content = "[User] " + message;
    user_item.metadata = "{}";
    recall_manager.add_recall_memory(user_item);
    
    // 模拟AI响应
    std::string response_text = "这是一个模拟的AI响应。";
    
    // 保存AI响应到回忆记忆
    RecallMemoryManager::RecallMemoryItem ai_item;
    ai_item.user_id = user_id;
    ai_item.agent_id = agent_id;
    ai_item.session_id = session_id;
    ai_item.message_type = "ai_response";
    ai_item.content = "[AI] " + response_text;
    ai_item.metadata = "{}";
    recall_manager.add_recall_memory(ai_item);
    
    return response_text;
}