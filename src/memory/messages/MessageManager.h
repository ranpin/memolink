#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include "../../database/managers/MemoryOrchestrator.h"
#include <string>
#include <vector>

class MessageManager {
public:
    struct Message {
        int id;
        int user_id;
        int agent_id;
        int session_id;
        std::string message_type;
        std::string content;
        std::string metadata;
        std::string embedding;
        std::string created_at;
        std::string updated_at;
        bool processed;
    };

    // 添加消息
    static bool addMessage(const Message& message, MemoryOrchestrator& orchestrator);
    
    // 获取未处理的消息
    static std::vector<Message> getUnprocessedMessages(int user_id, MemoryOrchestrator& orchestrator);
    
    // 根据会话获取消息
    static std::vector<Message> getMessagesBySession(int session_id, MemoryOrchestrator& orchestrator);
    
    // 标记消息为已处理
    static bool markMessagesAsProcessed(const std::vector<int>& message_ids, MemoryOrchestrator& orchestrator);
    
    // 获取未处理消息数量
    static int getUnprocessedMessageCount(int user_id, MemoryOrchestrator& orchestrator);
};

#endif // MESSAGE_MANAGER_H