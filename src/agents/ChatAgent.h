#ifndef CHAT_AGENT_H
#define CHAT_AGENT_H

#include "../database/core/StorageManager.h"
#include "../database/managers/RecallMemoryManager.h"
#include <string>

class ChatAgent {
private:
    StorageManager& storage_;
    int user_id_;
    int agent_id_;

public:
    explicit ChatAgent(StorageManager& storage);
    void initialize(int userId, int agentId);
    
    std::string buildContext(int user_id, int agent_id, int session_id, const std::string& message);
    std::string chat(int user_id, int agent_id, int session_id, const std::string& message);
};

#endif // CHAT_AGENT_H