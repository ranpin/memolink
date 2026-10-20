#pragma once
#include <algorithm>

// CommandLineInterface - 命令行界面，提供与用户交互的命令行接口

#include "../database/core/StorageManager.h"
#include "../models/ModelManager.h"
#include "../agents/AgentManager.h"
#include "../api/APIServer.h"
#include <string>
#include <memory>

class CommandLineInterface {
private:
    StorageManager& storage_;
    ModelManager model_manager_;
    std::unique_ptr<AgentManager> agent_manager_;
    std::unique_ptr<APIServer> api_server_;
    bool running_;

public:
    CommandLineInterface(StorageManager& storage);
    
    // 主循环接口
    void start();
    void stop();
    
    // 命令处理接口
    void process_command(const std::string& command);
    
    // 用户交互接口
    void show_help();
    void show_status();
    std::string get_user_input();
    
    // 聊天处理接口
    void handle_chat(const std::string& message);
};