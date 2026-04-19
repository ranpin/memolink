#include "MessageManager.h"
#include <iostream>
#include <sstream>
#include "../../database/managers/MessageMemoryManager.h"

bool MessageManager::addMessage(const Message& message, MemoryOrchestrator& orchestrator) {
    try {
        // 从orchestrator获取MessageMemoryManager
        auto& manager = orchestrator.getMessageMemoryManager();
        
        MessageMemory msg;
        msg.user_id = message.user_id;
        msg.agent_id = message.agent_id;
        msg.session_id = message.session_id;
        msg.content = message.content;
        // 设置时间戳
        msg.created_at = "";
        msg.processed = false;
        
        int64_t result = manager.add(msg);
        return result > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error saving message: " + std::string(e.what()) << std::endl;
        return false;
    }
}

std::vector<MessageManager::Message> MessageManager::getUnprocessedMessages(int user_id, MemoryOrchestrator& orchestrator) {
    try {
        // 从orchestrator获取MessageMemoryManager
        auto& manager = orchestrator.getMessageMemoryManager();
        
        // 查询指定用户的未处理消息
        auto messages = manager.find(sqlite_orm::where(
            sqlite_orm::c(&MessageMemory::user_id) == user_id and
            sqlite_orm::c(&MessageMemory::processed) == 0
        ));
        
        std::vector<Message> result;
        for (const auto& msg : messages) {
            result.push_back({
                msg.id,
                msg.user_id,
                msg.agent_id,
                msg.session_id,
                "text", // message_type
                msg.content,
                "",  // metadata
                "", // embedding
                msg.created_at,
                "",  // updated_at
                static_cast<bool>(msg.processed)
            });
        }
        std::cout << "[MessageManager] Retrieved " << result.size() << " unprocessed messages" << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error getting unprocessed messages: " << e.what() << std::endl;
        return {};
    }
}

std::vector<MessageManager::Message> MessageManager::getMessagesBySession(int session_id, MemoryOrchestrator& orchestrator) {
    try {
        // 从orchestrator获取MessageMemoryManager
        auto& manager = orchestrator.getMessageMemoryManager();
        
        auto messages = manager.find(sqlite_orm::where(sqlite_orm::c(&MessageMemory::session_id) == session_id));
        
        std::vector<Message> result;
        for (const auto& msg : messages) {
            result.push_back({
                msg.id,
                msg.user_id,
                msg.agent_id,
                msg.session_id,
                "text", // message_type
                msg.content,
                "",  // metadata
                "", // embedding
                msg.created_at,
                "",  // updated_at
                false // processed
            });
        }
        std::cout << "[MessageManager] Retrieved " << result.size() << " messages for session " << session_id << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Error getting messages by session: " << e.what() << std::endl;
        return {};
    }
}

bool MessageManager::markMessagesAsProcessed(const std::vector<int>& message_ids, MemoryOrchestrator& orchestrator) {
    try {
        auto& manager = orchestrator.getMessageMemoryManager();
        
        for (int id : message_ids) {
            auto existing = manager.getById(id);
            if (!existing.has_value()) {
                continue;
            }
            MessageMemory message = existing.value();
            message.processed = true;
            manager.update(message);
        }
        
        std::cout << "[MessageManager] Marked " << message_ids.size() << " messages as processed" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error marking messages as processed: " << e.what() << std::endl;
        return false;
    }
}

int MessageManager::getUnprocessedMessageCount(int user_id, MemoryOrchestrator& orchestrator) {
    // 获取未处理消息的数量
    auto messages = getUnprocessedMessages(user_id, orchestrator);
    return static_cast<int>(messages.size());
}