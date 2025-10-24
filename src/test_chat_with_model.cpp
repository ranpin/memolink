#include "models/ModelManager.h"
#include "database/core/StorageManager.h"
#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "models/VectorSearchManager.h"
#include "agents/ChatAgent.h"
#include "utils/TimeUtils.h"
#include "database/initialization/InitDB.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sqlite3.h>

void print_separator() {
    std::cout << "\n" << std::string(50, '=') << "\n" << std::endl;
}

void print_sub_separator() {
    std::cout << "\n" << std::string(30, '-') << "\n" << std::endl;
}

class ChatWithModelTest {
private:
    StorageManager storage_;
    ModelManager model_;
    VectorSearchManager vector_search_manager_;
    int user_id_;
    int session_id_;
    int agent_id_;

public:
    ChatWithModelTest(const std::string& db_path) 
        : storage_(db_path), model_(storage_), vector_search_manager_(storage_), 
          user_id_(-1), session_id_(-1), agent_id_(-1) {
        // 初始化数据库
        if (!InitDB::initializeDatabase(db_path)) {
            throw std::runtime_error("Failed to initialize database");
        }
        
        if (!storage_.initialize()) {
            throw std::runtime_error("Failed to initialize storage manager");
        }
    }
    
    ~ChatWithModelTest() {
        // 结束会话
        if (session_id_ > 0) {
            SessionManager session_manager(storage_);
            session_manager.end_session(session_id_);
        }
    }
    
    void initialize_user_and_session() {
        std::cout << "初始化用户和会话..." << std::endl;
        
        // 创建测试agent
        agent_id_ = 1; // 使用默认agent
        
        // 检查用户是否已存在，如果不存在则创建
        user_id_ = UserManager::getUserIdByName("TestChatUser", storage_.getDatabasePath());
        if (user_id_ <= 0) {
            if (UserManager::addUser("TestChatUser", "testchat@example.com", "", storage_.getDatabasePath())) {
                std::cout << "创建测试用户成功" << std::endl;
                user_id_ = UserManager::getUserIdByName("TestChatUser", storage_.getDatabasePath());
            } else {
                throw std::runtime_error("创建测试用户失败");
            }
        } else {
            std::cout << "使用已存在的用户，ID: " << user_id_ << std::endl;
        }
        
        // 确保agent存在
        sqlite3_stmt* stmt;
        std::string checkAgentQuery = "SELECT id FROM agent WHERE id = 1;";
        if (storage_.executeQuery(checkAgentQuery, stmt)) {
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (rc != SQLITE_ROW) {
                // 创建默认agent
                std::string createAgentQuery = "INSERT OR IGNORE INTO agent (id, name, description) VALUES (1, 'Default Agent', '默认测试代理');";
                if (!storage_.executeNonQuery(createAgentQuery)) {
                    throw std::runtime_error("创建默认agent失败");
                }
            }
        }
        
        // 创建会话
        SessionManager session_manager(storage_);
        SessionItem session_item;
        session_item.user_id = std::to_string(user_id_);
        session_item.start_time = TimeUtils::getCurrentTimestamp();
        session_item.status = "active";
        session_item.metadata = "{}";
        
        if (session_manager.create_session(session_item)) {
            // 获取最新创建的会话ID
            auto sessions = session_manager.list_sessions(session_item.user_id);
            if (!sessions.empty()) {
                session_id_ = sessions.back().id;
                std::cout << "会话创建成功，ID: " << session_id_ << std::endl;
            } else {
                session_id_ = 1;
                std::cout << "会话创建成功，ID: " << session_id_ << std::endl;
            }
        } else {
            throw std::runtime_error("创建会话失败");
        }
    }
    
    // 构建上下文信息，包括历史对话、图片描述和相关记忆
    std::string build_context(const std::string& user_input) {
        std::cout << "\n=== 开始构建上下文 ===" << std::endl;
        std::ostringstream context_stream;
        
        // 1. 获取历史对话信息
        std::cout << "获取历史对话信息..." << std::endl;
        context_stream << "=== 历史对话信息 ===\n";
        RecallMemoryManager recall_manager(storage_);
        auto recent_memories = recall_manager.list_recall_memories(user_id_, 5); // 获取最近5条记录
        std::cout << "找到 " << recent_memories.size() << " 条历史对话记录" << std::endl;
        
        for (const auto& memory : recent_memories) {
            context_stream << "[" << memory.created_at << "] [" << memory.message_type << "] " 
                          << memory.content << "\n";
        }
        
        // 2. 获取相关记忆信息（基于向量搜索）
        std::cout << "获取相关记忆信息..." << std::endl;
        context_stream << "\n=== 相关记忆信息 ===\n";
        try {
            std::cout << "开始混合搜索..." << std::endl;
            auto relevant_memories = vector_search_manager_.hybridSearch(user_id_, user_input, 3);
            std::cout << "混合搜索完成，找到 " << relevant_memories.size() << " 条相关记忆" << std::endl;
            for (const auto& memory : relevant_memories) {
                context_stream << "[" << memory.timestamp << "] " << memory.content << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "搜索相关记忆时出错: " << e.what() << "\n";
            context_stream << "搜索相关记忆时出错: " << e.what() << "\n";
        }
        
        // 3. 获取图片描述信息
        std::cout << "获取图片资源信息..." << std::endl;
        context_stream << "\n=== 图片资源信息 ===\n";
        ResourceMemoryManager resource_manager(storage_);
        std::cout << "调用getResourcesByType获取图片资源..." << std::endl;
        auto image_resources = resource_manager.getResourcesByType(user_id_, "image");
        std::cout << "找到 " << image_resources.size() << " 个图片资源" << std::endl;
        for (const auto& resource : image_resources) {
            context_stream << "[" << resource.created_at << "] " << resource.title 
                          << " - " << resource.summary << "\n";
        }
        
        std::cout << "=== 上下文构建完成 ===" << std::endl;
        return context_stream.str();
    }
    
    // 读取图片文件并添加到资源记忆中
    void add_image_to_memory(const std::string& image_path) {
        std::cout << "=== 图片处理开始 ===" << std::endl;
        std::cout << "原始图片路径: " << image_path << std::endl;
        
        // 处理相对路径和绝对路径
        std::string full_path = image_path;
        std::cout << "当前工作目录: " << std::filesystem::current_path() << std::endl;
        
        if (!std::filesystem::path(image_path).is_absolute()) {
            // 如果是相对路径，则基于当前工作目录构建完整路径
            full_path = (std::filesystem::current_path() / image_path).string();
            std::cout << "检测到相对路径，转换为绝对路径: " << full_path << std::endl;
        } else {
            std::cout << "检测到绝对路径: " << full_path << std::endl;
        }
        
        std::cout << "检查文件是否存在: " << full_path << std::endl;
        
        // 检查文件是否存在
        if (!std::filesystem::exists(full_path)) {
            std::cerr << "无法打开图片文件: " << image_path << std::endl;
            std::cerr << "文件不存在或无法访问: " << full_path << std::endl;
            std::cerr << "请确保文件路径正确，例如:" << std::endl;
            std::cerr << "  macOS/Linux绝对路径: /Users/username/Pictures/sample.jpg" << std::endl;
            std::cerr << "  Windows绝对路径: C:\\Users\\username\\Pictures\\sample.jpg" << std::endl;
            std::cerr << "  相对路径: test_images/sample.jpg (相对于当前工作目录)" << std::endl;
            std::cout << "=== 图片处理结束(失败) ===" << std::endl;
            return;
        }
        
        std::cout << "文件存在，检查是否为常规文件" << std::endl;
        
        // 检查是否为常规文件
        if (!std::filesystem::is_regular_file(full_path)) {
            std::cerr << "路径不是常规文件: " << full_path << std::endl;
            std::cout << "=== 图片处理结束(失败) ===" << std::endl;
            return;
        }
        
        // 获取文件大小
        std::error_code ec;
        auto file_size = std::filesystem::file_size(full_path, ec);
        if (ec) {
            std::cerr << "无法读取图片文件大小: " << full_path << ", 错误: " << ec.message() << std::endl;
            std::cout << "=== 图片处理结束(失败) ===" << std::endl;
            return;
        }
        
        std::cout << "文件大小: " << file_size << " 字节" << std::endl;
        
        // 添加到资源记忆
        ResourceMemoryManager resource_manager(storage_);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = user_id_;
        item.agent_id = agent_id_; // 使用已初始化的agent ID
        item.filename = std::filesystem::path(full_path).filename().string();
        item.title = "上传图片: " + item.filename;
        item.content = "用户上传的图片文件";
        item.resource_type = "image";
        item.summary = "图片文件，路径: " + full_path;
        item.size = static_cast<int>(file_size);
        item.tree_path = "/";
        item.path = full_path;
        item.metadata = "{}";
        
        std::cout << "准备添加资源记忆项..." << std::endl;
        std::cout << "  用户ID: " << item.user_id << std::endl;
        std::cout << "  Agent ID: " << item.agent_id << std::endl;
        std::cout << "  文件名: " << item.filename << std::endl;
        std::cout << "  标题: " << item.title << std::endl;
        std::cout << "  资源类型: " << item.resource_type << std::endl;
        std::cout << "  摘要: " << item.summary << std::endl;
        std::cout << "  大小: " << item.size << std::endl;
        std::cout << "  路径: " << item.path << std::endl;
        
        if (resource_manager.add_resource_memory(item)) {
            std::cout << "图片已添加到资源记忆中" << std::endl;
        } else {
            std::cerr << "添加图片到资源记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        std::cout << "=== 图片处理结束 ===" << std::endl;
    }

    // 处理用户输入并获取模型响应
    std::string process_user_input(const std::string& user_input, const std::string& image_path = "") {
        std::cout << "\n=== 处理用户输入开始 ===" << std::endl;
        std::cout << "用户输入: " << user_input << std::endl;
        std::cout << "图片路径: " << image_path << std::endl;
        
        // 如果提供了图片路径，添加到资源记忆中
        if (!image_path.empty()) {
            std::cout << "处理图片: " << image_path << std::endl;
            add_image_to_memory(image_path);
        }
        
        // 构建上下文
        std::string context = build_context(user_input);
        std::cout << "\n--- 上下文信息 ---\n" << context << "\n--- 上下文信息结束 ---" << std::endl;
        
        // 保存用户消息到回忆记忆
        RecallMemoryManager recall_manager(storage_);
        RecallMemoryManager::RecallMemoryItem user_item;
        user_item.user_id = user_id_;
        user_item.agent_id = agent_id_; // 使用已初始化的agent ID
        user_item.session_id = session_id_;
        user_item.message_type = "user_message";
        user_item.content = user_input;
        user_item.metadata = "{}";
        
        std::cout << "保存用户消息到回忆记忆..." << std::endl;
        if (!recall_manager.add_recall_memory(user_item)) {
            std::cerr << "保存用户消息到回忆记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        // 调用模型获取响应
        std::string model_response;
        std::cout << "调用模型获取响应..." << std::endl;
        if (!image_path.empty()) {
            // 多模态处理
            std::cout << "使用多模态处理..." << std::endl;
            model_response = model_.process_multimodal(user_input, image_path);
        } else {
            // 文本处理
            std::cout << "使用文本处理..." << std::endl;
            std::vector<std::string> context_lines = {context};
            model_response = model_.generateResponse(context_lines, user_input);
        }
        
        std::cout << "模型响应: " << model_response << std::endl;
        
        // 保存模型响应到回忆记忆
        RecallMemoryManager::RecallMemoryItem ai_item;
        ai_item.user_id = user_id_;
        ai_item.agent_id = agent_id_; // 使用已初始化的agent ID
        ai_item.session_id = session_id_;
        ai_item.message_type = "ai_response";
        ai_item.content = model_response;
        ai_item.metadata = "{}";
        
        std::cout << "保存AI响应到回忆记忆..." << std::endl;
        if (!recall_manager.add_recall_memory(ai_item)) {
            std::cerr << "保存AI响应到回忆记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        std::cout << "=== 处理用户输入结束 ===" << std::endl;
        
        return model_response;
    }
    
    void run_chat_loop() {
        std::string user_input;
        std::cout << "\n=== MIRIX 模型对话测试 ===" << std::endl;
        std::cout << "输入消息与模型对话 (输入 'quit' 退出, 'help' 查看帮助):" << std::endl;
        
        while (true) {
            std::cout << "\n用户: ";
            if (!std::getline(std::cin, user_input)) {
                break;
            }
            
            // 清理输入
            user_input.erase(0, user_input.find_first_not_of(" \t\r\n"));
            user_input.erase(user_input.find_last_not_of(" \t\r\n") + 1);
            
            if (user_input.empty()) {
                continue;
            }
            
            if (user_input == "quit" || user_input == "exit") {
                std::cout << "再见!" << std::endl;
                break;
            }
            
            if (user_input == "help") {
                print_help();
                continue;
            }
            
            // 处理特殊命令
            if (user_input.substr(0, 6) == "/image") {
                std::string image_path = user_input.substr(7);
                std::cout << "请输入关于这张图片的问题: ";
                std::string question;
                std::getline(std::cin, question);
                
                std::string response = process_user_input(question, image_path);
                std::cout << "AI: " << response << std::endl;
                continue;
            }
            
            // 处理普通文本输入
            std::string response = process_user_input(user_input);
            std::cout << "AI: " << response << std::endl;
        }
    }
    
    void print_help() {
        std::cout << "\n=== 帮助信息 ===" << std::endl;
        std::cout << "命令列表:" << std::endl;
        std::cout << "  help             - 显示帮助信息" << std::endl;
        std::cout << "  quit 或 exit     - 退出程序" << std::endl;
        std::cout << "  /image <路径>    - 上传图片并进行问答" << std::endl;
        std::cout << "  <文本>           - 发送文本消息" << std::endl;
        std::cout << "\n程序会自动:" << std::endl;
        std::cout << "  1. 将对话历史保存到数据库" << std::endl;
        std::cout << "  2. 检索相关记忆作为上下文" << std::endl;
        std::cout << "  3. 显示所有中间信息" << std::endl;
        std::cout << "  4. 调用模型获取响应" << std::endl;
        std::cout << "  5. 将响应保存到数据库" << std::endl;
    }
};

// 构建上下文信息，包括历史对话、图片描述和相关记忆
std::string build_context(StorageManager& storage, VectorSearchManager& vector_search_manager, int user_id, const std::string& user_input) {
    std::ostringstream context_stream;
    
    // 1. 获取历史对话信息
    context_stream << "=== 历史对话信息 ===\n";
    RecallMemoryManager recall_manager(storage);
    auto recent_memories = recall_manager.list_recall_memories(user_id, 5); // 获取最近5条记录
    
    for (const auto& memory : recent_memories) {
        context_stream << "[" << memory.created_at << "] [" << memory.message_type << "] " 
                      << memory.content << "\n";
    }
    
    // 2. 获取相关记忆信息（基于向量搜索）
    context_stream << "\n=== 相关记忆信息 ===\n";
    try {
        auto relevant_memories = vector_search_manager.hybridSearch(user_id, user_input, 3);
        for (const auto& memory : relevant_memories) {
            context_stream << "[" << memory.timestamp << "] " << memory.content << "\n";
        }
    } catch (const std::exception& e) {
        context_stream << "搜索相关记忆时出错: " << e.what() << "\n";
    }
    
    // 3. 获取图片描述信息
    context_stream << "\n=== 图片资源信息 ===\n";
    ResourceMemoryManager resource_manager(storage);
    auto image_resources = resource_manager.getResourcesByType(user_id, "image");
    for (const auto& resource : image_resources) {
        context_stream << "[" << resource.created_at << "] " << resource.title 
                      << " - " << resource.summary << "\n";
    }
    
    return context_stream.str();
}

// 读取图片文件并添加到资源记忆中
void add_image_to_memory(StorageManager& storage, int user_id, int agent_id, const std::string& image_path) {
    std::cout << "=== 图片处理开始 ===" << std::endl;
    std::cout << "原始图片路径: " << image_path << std::endl;
    
    // 处理相对路径和绝对路径
    std::string full_path = image_path;
    std::cout << "当前工作目录: " << std::filesystem::current_path() << std::endl;
    
    if (!std::filesystem::path(image_path).is_absolute()) {
        // 如果是相对路径，则基于当前工作目录构建完整路径
        full_path = (std::filesystem::current_path() / image_path).string();
        std::cout << "检测到相对路径，转换为绝对路径: " << full_path << std::endl;
    } else {
        std::cout << "检测到绝对路径: " << full_path << std::endl;
    }
    
    std::cout << "检查文件是否存在: " << full_path << std::endl;
    
    // 检查文件是否存在
    if (!std::filesystem::exists(full_path)) {
        std::cerr << "无法打开图片文件: " << image_path << std::endl;
        std::cerr << "文件不存在或无法访问: " << full_path << std::endl;
        std::cerr << "请确保文件路径正确，例如:" << std::endl;
        std::cerr << "  macOS/Linux绝对路径: /Users/username/Pictures/sample.jpg" << std::endl;
        std::cerr << "  Windows绝对路径: C:\\Users\\username\\Pictures\\sample.jpg" << std::endl;
        std::cerr << "  相对路径: test_images/sample.jpg (相对于当前工作目录)" << std::endl;
        std::cout << "=== 图片处理结束(失败) ===" << std::endl;
        return;
    }
    
    std::cout << "文件存在，检查是否为常规文件" << std::endl;
    
    // 检查是否为常规文件
    if (!std::filesystem::is_regular_file(full_path)) {
        std::cerr << "路径不是常规文件: " << full_path << std::endl;
        std::cout << "=== 图片处理结束(失败) ===" << std::endl;
        return;
    }
    
    // 获取文件大小
    std::error_code ec;
    auto file_size = std::filesystem::file_size(full_path, ec);
    if (ec) {
        std::cerr << "无法读取图片文件大小: " << full_path << ", 错误: " << ec.message() << std::endl;
        std::cout << "=== 图片处理结束(失败) ===" << std::endl;
        return;
    }
    
    std::cout << "文件大小: " << file_size << " 字节" << std::endl;
    
    // 添加到资源记忆
    ResourceMemoryManager resource_manager(storage);
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = agent_id;
    item.filename = std::filesystem::path(full_path).filename().string();
    item.title = "上传图片: " + item.filename;
    item.content = "用户上传的图片文件";
    item.resource_type = "image";
    item.summary = "图片文件，路径: " + full_path;
    item.size = static_cast<int>(file_size);
    item.tree_path = "/";
    item.path = full_path;
    item.metadata = "{}";
    
    std::cout << "准备添加资源记忆项..." << std::endl;
    std::cout << "  用户ID: " << item.user_id << std::endl;
    std::cout << "  Agent ID: " << item.agent_id << std::endl;
    std::cout << "  文件名: " << item.filename << std::endl;
    std::cout << "  标题: " << item.title << std::endl;
    std::cout << "  资源类型: " << item.resource_type << std::endl;
    std::cout << "  摘要: " << item.summary << std::endl;
    std::cout << "  大小: " << item.size << std::endl;
    std::cout << "  路径: " << item.path << std::endl;
    
    if (resource_manager.add_resource_memory(item)) {
        std::cout << "图片已添加到资源记忆中" << std::endl;
    } else {
        std::cerr << "添加图片到资源记忆失败: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
    }
    
    std::cout << "=== 图片处理结束 ===" << std::endl;
}

int main() {
    try {
        std::cout << "=== MIRIX 模型对话测试 ===" << std::endl;
        
        // 初始化存储管理器
        StorageManager storage("test_chat_with_model.db");
        if (!storage.initialize()) {
            throw std::runtime_error("数据库初始化失败");
        }
        
        // 初始化数据库表
        InitDB::initializeDatabase("test_chat_with_model.db");
        
        // 初始化向量搜索管理器
        VectorSearchManager vector_search_manager(storage);
        
        // 初始化模型管理器
        ModelManager model(storage);
        
        // 创建测试用户和会话
        std::cout << "初始化用户和会话..." << std::endl;
        int user_id = 1; // 简单测试使用固定用户ID
        int agent_id = 1; // 简单测试使用固定Agent ID
        
        // 创建用户表记录（如果不存在）
        sqlite3_stmt* stmt;
        std::string user_check_query = "SELECT id FROM user WHERE id = 1;";
        if (storage.executeQuery(user_check_query, stmt)) {
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (result != SQLITE_ROW) {
                // 用户不存在，创建测试用户
                std::string create_user_query = "INSERT OR IGNORE INTO user (id, name, email, created_at) VALUES (1, 'test_user', 'test@example.com', '2023-01-01 00:00:00');";
                if (!storage.executeNonQuery(create_user_query)) {
                    throw std::runtime_error("创建测试用户失败");
                }
                std::cout << "创建测试用户成功" << std::endl;
            } else {
                std::cout << "使用现有测试用户" << std::endl;
            }
        }
        
        // 创建Agent
        std::string agent_check_query = "SELECT id FROM agent WHERE id = 1;";
        if (storage.executeQuery(agent_check_query, stmt)) {
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (result != SQLITE_ROW) {
                // Agent不存在，创建测试Agent
                std::string create_agent_query = "INSERT OR IGNORE INTO agent (id, name, description, capabilities, created_at) VALUES (1, 'test_agent', 'Test Agent for demo', 'general', '2023-01-01 00:00:00');";
                if (!storage.executeNonQuery(create_agent_query)) {
                    throw std::runtime_error("创建测试Agent失败");
                }
                std::cout << "创建测试Agent成功" << std::endl;
            } else {
                std::cout << "使用现有测试Agent" << std::endl;
            }
        }
        
        // 创建会话
        int session_id = 1;
        std::string session_check_query = "SELECT id FROM session WHERE id = 1;";
        if (storage.executeQuery(session_check_query, stmt)) {
            int result = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (result != SQLITE_ROW) {
                // 会话不存在，创建测试会话
                std::string create_session_query = "INSERT OR IGNORE INTO session (id, user_id, start_time, status, metadata) VALUES (1, 1, '2023-01-01 00:00:00', 'active', '{}');";
                if (!storage.executeNonQuery(create_session_query)) {
                    throw std::runtime_error("创建会话失败");
                }
                std::cout << "会话创建成功，ID: " << session_id << std::endl;
            } else {
                std::cout << "使用现有会话，ID: " << session_id << std::endl;
            }
        }
        
        // 主循环
        std::string user_input;
        std::cout << "\n输入消息与模型对话 (输入 'quit' 退出, 'help' 查看帮助):" << std::endl;
        
        while (true) {
            std::cout << "\n用户: ";
            std::getline(std::cin, user_input);
            
            if (user_input == "quit") {
                std::cout << "再见!" << std::endl;
                break;
            }
            
            if (user_input == "help") {
                std::cout << "\n帮助信息:" << std::endl;
                std::cout << "  直接输入文本与模型对话" << std::endl;
                std::cout << "  输入 '/image <图片路径>' 添加图片并询问相关问题" << std::endl;
                std::cout << "  输入 'quit' 退出程序" << std::endl;
                continue;
            }
            
            // 处理图片输入
            if (user_input.substr(0, 6) == "/image") {
                std::string image_path = user_input.substr(7); // 去掉 "/image " 前缀
                
                if (image_path.empty()) {
                    std::cout << "请提供图片路径，例如: /image /path/to/image.jpg" << std::endl;
                    continue;
                }
                
                // 请求用户输入关于图片的问题
                std::cout << "请输入关于这张图片的问题: ";
                std::string image_question;
                std::getline(std::cin, image_question);
                
                if (!image_question.empty()) {
                    // 构建上下文
                    std::string context = build_context(storage, vector_search_manager, user_id, image_question);
                    std::cout << "\n--- 上下文信息 ---\n" << context << "\n--- 上下文信息结束 ---" << std::endl;
                    
                    // 保存用户消息到回忆记忆
                    RecallMemoryManager recall_manager(storage);
                    RecallMemoryManager::RecallMemoryItem user_item;
                    user_item.user_id = user_id;
                    user_item.agent_id = agent_id;
                    user_item.session_id = session_id;
                    user_item.message_type = "user_message";
                    user_item.content = image_question;
                    user_item.metadata = "{}";
                    
                    if (!recall_manager.add_recall_memory(user_item)) {
                        std::cerr << "保存用户消息到回忆记忆失败: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
                    }
                    
                    // 处理图片
                    add_image_to_memory(storage, user_id, agent_id, image_path);
                    
                    // 调用多模态模型
                    std::string model_response = model.process_multimodal(image_question, image_path);
                    std::cout << "AI: " << model_response << std::endl;
                    
                    // 保存模型响应到回忆记忆
                    RecallMemoryManager::RecallMemoryItem ai_item;
                    ai_item.user_id = user_id;
                    ai_item.agent_id = agent_id;
                    ai_item.session_id = session_id;
                    ai_item.message_type = "ai_response";
                    ai_item.content = model_response;
                    ai_item.metadata = "{}";
                    
                    if (!recall_manager.add_recall_memory(ai_item)) {
                        std::cerr << "保存AI响应到回忆记忆失败: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
                    }
                } else {
                    std::cout << "未提供问题，跳过图片处理。" << std::endl;
                }
            } else {
                // 处理普通文本输入
                // 构建上下文
                std::string context = build_context(storage, vector_search_manager, user_id, user_input);
                std::cout << "\n--- 上下文信息 ---\n" << context << "\n--- 上下文信息结束 ---" << std::endl;
                
                // 保存用户消息到回忆记忆
                RecallMemoryManager recall_manager(storage);
                RecallMemoryManager::RecallMemoryItem user_item;
                user_item.user_id = user_id;
                user_item.agent_id = agent_id;
                user_item.session_id = session_id;
                user_item.message_type = "user_message";
                user_item.content = user_input;
                user_item.metadata = "{}";
                
                if (!recall_manager.add_recall_memory(user_item)) {
                    std::cerr << "保存用户消息到回忆记忆失败: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
                }
                
                // 调用模型
                std::vector<std::string> context_lines = {context};
                std::string model_response = model.generateResponse(context_lines, user_input);
                std::cout << "AI: " << model_response << std::endl;
                
                // 保存模型响应到回忆记忆
                RecallMemoryManager::RecallMemoryItem ai_item;
                ai_item.user_id = user_id;
                ai_item.agent_id = agent_id;
                ai_item.session_id = session_id;
                ai_item.message_type = "ai_response";
                ai_item.content = model_response;
                ai_item.metadata = "{}";
                
                if (!recall_manager.add_recall_memory(ai_item)) {
                    std::cerr << "保存AI响应到回忆记忆失败: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
                }
            }
        }
        
        std::cout << "\n测试完成，数据已保存到: test_chat_with_model.db" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
