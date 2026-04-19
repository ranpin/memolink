#ifndef COMMAND_LINE_INTERFACE_H
#define COMMAND_LINE_INTERFACE_H

#include "../database/core/StorageManager.h"
#include "../models/ModelManager.h"
#include "../agents/ChatAgent.h"
#include "../agents/MemoryAgent.h"
#include <string>
#include <memory>

class CommandLineInterface {
private:
    StorageManager& storage_;
    ModelManager model_manager_;
    // std::unique_ptr<AgentManager> agent_manager_;
    std::unique_ptr<ChatAgent> chat_agent_;
    std::unique_ptr<MemoryAgent> memory_agent_;
    bool initialized_;
    
    // 用于存储上次检查的孤儿 embedding ID
    std::vector<int> last_orphaned_ids_;

public:
    explicit CommandLineInterface(StorageManager& storage);
    ~CommandLineInterface();
    
    void initialize();
    void run();
    void show_help();
    
private:
    void handle_user_input(const std::string& input);
    void chat_mode();
    void memory_mode();

    // 调试与工具命令
    void show_context();
    void list_memories(const std::string& type);
    void clear_all_memories();
    void check_index_consistency();
    void clean_orphaned_embeddings();
};

#endif // COMMAND_LINE_INTERFACE_H