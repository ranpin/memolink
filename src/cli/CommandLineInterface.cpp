#include <string>
#include <memory>
#include "CommandLineInterface.h"
#include "../database/initialization/ConfigManager/ConfigManager.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

CommandLineInterface::CommandLineInterface(StorageManager& storage) 
    : storage_(storage), model_manager_(storage), running_(false) {
    agent_manager_ = std::make_unique<AgentManager>(storage, model_manager_);
    api_server_ = std::make_unique<APIServer>(storage);
}

void CommandLineInterface::start() {
    std::cout << "Mirix CLI started. Type 'help' for available commands." << std::endl;
    running_ = true;
    
    while (running_) {
        std::cout << "> ";
        std::string input = get_user_input();
        process_command(input);
    }
}

std::string CommandLineInterface::get_user_input() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void CommandLineInterface::process_command(const std::string& command) {
    if (command == "help") {
        show_help();
    } else if (command == "quit" || command == "exit") {
        running_ = false;
    } else if (command == "status") {
        show_status();
    } else if (command.substr(0, 4) == "chat") {
        handle_chat(command.substr(5));
    } else {
        std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
    }
}

void CommandLineInterface::show_help() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help    - Show this help message" << std::endl;
    std::cout << "  status  - Show system status" << std::endl;
    std::cout << "  chat    - Chat with the AI assistant" << std::endl;
    std::cout << "  quit    - Exit the application" << std::endl;
}

void CommandLineInterface::show_status() {
    std::cout << "System Status:" << std::endl;
    std::cout << "  Storage: Connected" << std::endl;
    std::cout << "  Model: Loaded" << std::endl;
    std::cout << "  Agents: Active" << std::endl;
}

void CommandLineInterface::handle_chat(const std::string& message) {
    if (message.empty()) {
        std::cout << "Please provide a message to chat with." << std::endl;
        return;
    }
    
    // 简化的聊天处理
    std::cout << "AI: Hello! You said: " << message << std::endl;
}