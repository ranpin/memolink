#ifndef CHAT_AGENT_H
#define CHAT_AGENT_H

#include "../database/managers/MemoryOrchestrator.h"
#include "../models/VectorSearchManager.h"
#include <string>
#include <memory>

class ChatAgent {
private:
    std::shared_ptr<MemoryOrchestrator> orchestrator_;
    std::unique_ptr<StorageManager> storage_manager_;  // 持有 StorageManager 的所有权
    std::unique_ptr<VectorSearchManager> vector_search_manager_;
    int user_id_;
    int agent_id_;

public:
    explicit ChatAgent(std::shared_ptr<MemoryOrchestrator> orchestrator);
    void initialize(int userId, int agentId);
    
    std::string buildContext(int user_id, int agent_id, int session_id, const std::string& message);
    std::string chat(int user_id, int agent_id, int session_id, const std::string& message);
};

#endif // CHAT_AGENT_H