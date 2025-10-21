#include <iostream>
#include <string>
#include <vector>

#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "database/managers/EpisodicMemoryManager.h"
#include "database/managers/SemanticMemoryManager.h"
#include "database/managers/ProceduralMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/managers/KnowledgeVaultManager.h"
#include "database/managers/ArchivalMemoryManager.h"
#include "database/managers/RecallMemoryManager.h"
#include "models/ModelManager.h"
#include "agents/AgentManager.h"
#include "agents/ChatAgent.h"
#include "agents/CoreMemoryAgent.h"
#include "agents/EpisodicMemoryAgent.h"
#include "agents/SemanticMemoryAgent.h"
#include "agents/ProceduralMemoryAgent.h"
#include "agents/ResourceMemoryAgent.h"
#include "agents/KnowledgeVaultAgent.h"
#include "utils/TimeUtils.h"

class InteractiveMIRIX {
private:
    StorageManager storage_;
    ModelManager model_;
    std::unique_ptr<AgentManager> agent_manager_;
    std::unique_ptr<ChatAgent> chat_agent_;
    int user_id_;
    int session_id_;
    int agent_id_;

public:
    InteractiveMIRIX(const std::string& db_path) 
        : storage_(db_path), model_(storage_), user_id_(-1), session_id_(-1), agent_id_(-1) {
        // 初始化数据库
        if (!InitDB::initializeDatabase(db_path)) {
            throw std::runtime_error("Failed to initialize database");
        }
        
        if (!storage_.initialize()) {
            throw std::runtime_error("Failed to initialize storage manager");
        }
    }
    
    ~InteractiveMIRIX() {
        // 析构时结束会话
        if (session_id_ > 0) {
            SessionManager session_manager(storage_);
            session_manager.end_session(session_id_);
        }
    }
    
    void initialize_user_and_session() {
        // 检查用户是否已存在，如果不存在则创建
        user_id_ = UserManager::getUserIdByName("InteractiveUser", storage_.getDatabasePath());
        if (user_id_ <= 0) {
            if (UserManager::addUser("InteractiveUser", "interactive@example.com", "", storage_.getDatabasePath())) {
                std::cout << "创建默认用户成功" << std::endl;
            } else {
                std::cerr << "创建默认用户失败" << std::endl;
            }

            // 获取新创建的用户ID
            user_id_ = UserManager::getUserIdByName("InteractiveUser", storage_.getDatabasePath());
        } else {
            std::cout << "Using existing user with ID: " << user_id_ << std::endl;
        }
        
        // 初始化代理管理器和聊天代理
        agent_manager_ = std::make_unique<AgentManager>(storage_, model_);
        chat_agent_ = std::make_unique<ChatAgent>(storage_);
        
        // 创建默认agent
        agent_id_ = agent_manager_->get_agent_id_by_name("DefaultAgent");
        if (agent_id_ <= 0) {
            agent_id_ = agent_manager_->create_agent("DefaultAgent", "general", "active");
            if (agent_id_ <= 0) {
                std::cerr << "Failed to create default agent" << std::endl;
                agent_id_ = 1; // 备用方案
            } else {
                std::cout << "Agent created with ID: " << agent_id_ << std::endl;
            }
        } else {
            std::cout << "Using existing agent with ID: " << agent_id_ << std::endl;
        }
        
        // 创建会话
        SessionManager session_manager(storage_);
        SessionItem session_item;
        session_item.user_id = std::to_string(user_id_);
        
        // 获取当前时间戳
        session_item.start_time = TimeUtils::getCurrentTimestamp();
        
        session_item.end_time = "";
        session_item.status = "active";
        session_item.metadata = "{}";
        
        // 正确获取创建的会话ID
        if (session_manager.create_session(session_item)) {
            // 获取最新创建的会话ID
            auto sessions = session_manager.list_sessions(session_item.user_id);
            if (!sessions.empty()) {
                session_id_ = sessions.back().id;  // 获取最后一个创建的会话
                std::cout << "Session created with ID: " << session_id_ << std::endl;
            } else {
                session_id_ = 1; // 备用方案
                std::cout << "Session created with ID: " << session_id_ << std::endl;
            }
        } else {
            session_id_ = -1;
            std::cout << "Failed to create session" << std::endl;
        }
    }
    
    void chat_loop() {
        std::string user_input;
        std::cout << "\n=== MIRIX Interactive Mode ===" << std::endl;
        std::cout << "Enter your messages (type 'quit' to exit, 'help' for commands):" << std::endl;
        
        while (true) {
            std::cout << "\nYou: ";
            if (!std::getline(std::cin, user_input)) {
                // EOF detected, exit gracefully
                std::cout << "\nGoodbye!" << std::endl;
                break;
            }
            
            // 清理输入中的空白字符
            user_input.erase(0, user_input.find_first_not_of(" \t\r\n"));
            user_input.erase(user_input.find_last_not_of(" \t\r\n") + 1);
            
            if (user_input.empty()) {
                continue;
            }
            
            // 检查退出命令
            if (user_input == "quit" || user_input == "exit") {
                std::cout << "Goodbye!" << std::endl;
                std::cout << "Thank you for using MIRIX!" << std::endl;
                break;
            }
            
            if (user_input == "help") {
                print_help();
                continue;
            }
            
            // 使用 ChatAgent 处理用户输入
            std::string response = chat_agent_->chat(user_id_, agent_id_, session_id_, user_input);
            std::cout << "MIRIX: " << response << std::endl;
        }
    }
    
    void print_help() {
        std::cout << "\n=== MIRIX Help ===\n";
        std::cout << "Available commands:\n";
        std::cout << "  help    - Show this help message\n";
        std::cout << "  quit    - Exit the program\n";
        std::cout << "  exit    - Exit the program\n";
        std::cout << "  <text>  - Send a message to MIRIX\n";
        std::cout << "\nMIRIX will remember your conversations and use them for context.\n\n";
    }
};

int main() {
    try {
        std::string db_path = "interactive_mirix.db";
        InteractiveMIRIX mirix(db_path);
        
        mirix.initialize_user_and_session();
        mirix.chat_loop();
        
        std::cout << "Thank you for using MIRIX!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}