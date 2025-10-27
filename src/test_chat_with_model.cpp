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
#include "utils/VectorSerializer.h"
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

class ChatModelTest {
private:
    StorageManager storage_;
    ModelManager model_;
    VectorSearchManager vector_search_manager_;
    int user_id_;
    int agent_id_;
    int session_id_;

public:
    ChatModelTest() : storage_("test_chat_with_model.db"), 
                     model_(storage_),
                     vector_search_manager_(storage_),
                     user_id_(-1),
                     agent_id_(-1),
                     session_id_(-1) {
        if (!storage_.initialize()) {
            throw std::runtime_error("数据库初始化失败");
        }
        
        // 使用项目已有的数据库初始化逻辑
        if (!InitDB::initializeDatabase("test_chat_with_model.db")) {
            throw std::runtime_error("数据库表初始化失败");
        }
        
        initialize_user_and_session();
    }
    
    ~ChatModelTest() {
        // 结束会话
        if (session_id_ > 0) {
            SessionManager session_manager(storage_);
            session_manager.end_session(session_id_);
        }
    }
    
    void initialize_user_and_session() {
        std::cout << "初始化用户和会话..." << std::endl;
        
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
        
        // 检查agent是否存在，如果不存在则创建
        sqlite3_stmt* stmt;
        std::string checkAgentQuery = "SELECT id FROM agent WHERE id = 1;";
        if (storage_.executeQuery(checkAgentQuery, stmt)) {
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (rc != SQLITE_ROW) {
                // 创建默认agent
                std::string createAgentQuery = "INSERT OR IGNORE INTO agent (id, name, description, capabilities, created_at) VALUES (1, 'Default Agent', '默认测试代理', 'general', '2023-01-01 00:00:00');";
                if (!storage_.executeNonQuery(createAgentQuery)) {
                    throw std::runtime_error("创建默认agent失败");
                }
                std::cout << "创建默认Agent成功" << std::endl;
                agent_id_ = 1;
            } else {
                std::cout << "使用已存在的Agent，ID: 1" << std::endl;
                agent_id_ = 1;
            }
        }
        
        // 每次运行都创建新的会话
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
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[构建上下文] 开始构建上下文信息..." << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::ostringstream context_stream;
        
        // 1. 获取历史对话信息（从最近几次会话中获取）
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[构建上下文] 阶段1: 获取历史对话信息..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        context_stream << "=== 历史对话信息 ===\n";
        RecallMemoryManager recall_manager(storage_);
        // 使用新的方法获取用户最近5次会话的历史记录，最多获取10条
        auto recent_memories = recall_manager.list_recent_sessions_memories(user_id_, 5, 10);
        
        std::cout << "[构建上下文]   获取到 " << recent_memories.size() << " 条历史对话记录" << std::endl;
        for (const auto& memory : recent_memories) {
            std::cout << "[构建上下文]     [" << memory.created_at << "] [" << memory.message_type << "] " 
                      << memory.content << std::endl;
            context_stream << "[" << memory.created_at << "] [" << memory.message_type << "] " 
                          << memory.content << "\n";
        }
        
        print_sub_separator();
        
        // 2. 获取相关记忆信息（基于向量搜索）
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[构建上下文] 阶段2: 执行向量混合搜索..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        context_stream << "\n=== 相关记忆信息 ===\n";
        try {
            std::cout << "[构建上下文]   2.1 文本向量化..." << std::endl;
            std::cout << "[构建上下文]     查询文本: \"" << user_input << "\"" << std::endl;
            
            std::cout << "\n[构建上下文]   2.2 相似性搜索..." << std::endl;
            auto relevant_memories = vector_search_manager_.hybridSearch(user_id_, user_input, 3);
            
            std::cout << "\n[构建上下文]   2.3 结果处理..." << std::endl;
            std::cout << "[构建上下文]     找到 " << relevant_memories.size() << " 条相关记忆:" << std::endl;
            for (const auto& memory : relevant_memories) {
                std::cout << "[构建上下文]       [" << memory.timestamp << "] " << memory.content << std::endl;
                context_stream << "[" << memory.timestamp << "] " << memory.content << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << "[构建上下文]   搜索相关记忆时出错: " << e.what() << std::endl;
            context_stream << "搜索相关记忆时出错: " << e.what() << "\n";
        }
        
        print_sub_separator();
        
        // 3. 获取图片描述信息
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[构建上下文] 阶段3: 获取图片资源信息..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        context_stream << "\n=== 图片资源信息 ===\n";
        ResourceMemoryManager resource_manager(storage_);
        // 遍历所有资源记忆，查找图片资源
        auto all_resources = resource_manager.list_resource_memories(user_id_);
        std::vector<ResourceMemoryManager::ResourceMemoryItem> image_resources;
        for (const auto& resource : all_resources) {
            if (resource.resource_type == "image") {
                image_resources.push_back(resource);
            }
        }
        
        std::cout << "[构建上下文]   找到 " << image_resources.size() << " 个图片资源:" << std::endl;
        for (const auto& resource : image_resources) {
            std::cout << "[构建上下文]     [" << resource.created_at << "] " << resource.path 
                      << " (" << resource.description << ")" << std::endl;
        }
        
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[构建上下文] 上下文构建完成" << std::endl;
        std::cout << "================================================================================\n" << std::endl;
        return context_stream.str();
    }
    
    // 读取图片文件并添加到资源记忆中
    void add_image_to_memory(const std::string& image_path) {
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[图片处理] 开始处理图片..." << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::cout << "[图片处理] 原始图片路径: " << image_path << std::endl;
        
        // 处理相对路径和绝对路径
        std::string full_path = image_path;
        std::cout << "[图片处理] 当前工作目录: " << std::filesystem::current_path() << std::endl;
        
        if (!std::filesystem::path(image_path).is_absolute()) {
            // 如果是相对路径，则基于当前工作目录构建完整路径
            full_path = (std::filesystem::current_path() / image_path).string();
            std::cout << "[图片处理] 检测到相对路径，转换为绝对路径: " << full_path << std::endl;
        } else {
            std::cout << "[图片处理] 检测到绝对路径: " << full_path << std::endl;
        }
        
        std::cout << "[图片处理] 检查文件是否存在: " << full_path << std::endl;
        
        // 检查文件是否存在
        if (!std::filesystem::exists(full_path)) {
            std::cerr << "[图片处理] 无法打开图片文件: " << image_path << std::endl;
            std::cerr << "[图片处理] 文件不存在或无法访问: " << full_path << std::endl;
            std::cerr << "[图片处理] 请确保文件路径正确，例如:" << std::endl;
            std::cerr << "[图片处理]   macOS/Linux绝对路径: /Users/username/Pictures/sample.jpg" << std::endl;
            std::cerr << "[图片处理]   Windows绝对路径: C:\\Users\\username\\Pictures\\sample.jpg" << std::endl;
            std::cerr << "[图片处理]   相对路径: test_images/sample.jpg (相对于当前工作目录)" << std::endl;
            std::cout << "\n================================================================================" << std::endl;
            std::cout << "[图片处理] 图片处理结束(失败)" << std::endl;
            std::cout << "================================================================================\n" << std::endl;
            return;
        }
        
        std::cout << "[图片处理] 文件存在，检查是否为常规文件" << std::endl;
        
        // 检查是否为常规文件
        if (!std::filesystem::is_regular_file(full_path)) {
            std::cerr << "[图片处理] 路径不是常规文件: " << full_path << std::endl;
            std::cout << "\n================================================================================" << std::endl;
            std::cout << "[图片处理] 图片处理结束(失败)" << std::endl;
            std::cout << "================================================================================\n" << std::endl;
            return;
        }
        
        // 获取文件大小
        std::error_code ec;
        auto file_size = std::filesystem::file_size(full_path, ec);
        if (ec) {
            std::cerr << "[图片处理] 无法读取图片文件大小: " << full_path << ", 错误: " << ec.message() << std::endl;
            std::cout << "\n================================================================================" << std::endl;
            std::cout << "[图片处理] 图片处理结束(失败)" << std::endl;
            std::cout << "================================================================================\n" << std::endl;
            return;
        }
        
        std::cout << "[图片处理] 文件大小: " << file_size << " 字节" << std::endl;
        
        // 添加到资源记忆
        ResourceMemoryManager resource_manager(storage_);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = user_id_;
        item.agent_id = agent_id_; // 使用已初始化的agent ID
        item.session_id = 1; // 默认session ID
        item.resource_type = "image";
        item.size = static_cast<int>(file_size);
        item.path = full_path;
        item.metadata = "{}";
        
        // 使用TextEmbeddingGenerator生成图像描述
        std::string filename = std::filesystem::path(full_path).filename().string();
        std::string generated_description = "这是一张图片，但未生成具体描述"; // 默认描述
        try {
            TextEmbeddingGenerator textEmbeddingGenerator(&storage_);
            generated_description = textEmbeddingGenerator.generateImageDescription(full_path);
            std::cout << "[图片处理] 图像描述生成成功: " << generated_description << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[图片处理] 图像描述生成失败: " << e.what() << std::endl;
            std::cout << "[图片处理] 使用默认描述" << std::endl;
        }
        
        item.description = generated_description; // 只使用生成的描述，不添加文件名前缀
        
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[图片处理] 准备添加资源记忆项..." << std::endl;
        std::cout << "[图片处理]   用户ID: " << item.user_id << std::endl;
        std::cout << "[图片处理]   Agent ID: " << item.agent_id << std::endl;
        std::cout << "[图片处理]   文件名: " << filename << std::endl;
        std::cout << "[图片处理]   描述: " << item.description << std::endl;
        std::cout << "[图片处理]   资源类型: " << item.resource_type << std::endl;
        std::cout << "[图片处理]   大小: " << item.size << std::endl;
        std::cout << "[图片处理]   路径: " << item.path << std::endl;
        
        // 为图像描述生成embedding并存储到资源记忆中
        try {
            TextEmbeddingGenerator textEmbeddingGenerator(&storage_);
            std::vector<float> descriptionEmbedding = textEmbeddingGenerator.generateEmbedding(generated_description);
            if (!descriptionEmbedding.empty()) {
                // 将embedding序列化并存储到item中
                item.embedding = VectorSerializer::serialize(descriptionEmbedding);
                std::cout << "[图片处理] 图像描述embedding生成成功，维度: " << descriptionEmbedding.size() << std::endl;
            } else {
                std::cout << "[图片处理] 图像描述embedding生成失败，返回空向量" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[图片处理] 图像描述embedding生成异常: " << e.what() << std::endl;
        }
        
        std::cout << "[图片处理] 准备存储的embedding大小: " << item.embedding.size() << " 字节" << std::endl;
        
        if (resource_manager.add_resource_memory(item)) {
            std::cout << "[图片处理] 图片已添加到资源记忆中" << std::endl;
        } else {
            std::cerr << "[图片处理] 添加图片到资源记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[图片处理] 图片处理结束" << std::endl;
        std::cout << "================================================================================\n" << std::endl;
    }

    // 处理用户输入并获取模型响应
    std::string process_user_input(const std::string& user_input, const std::string& image_path = "") {
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[处理输入] 开始处理用户输入..." << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::cout << "[处理输入] 用户输入: " << user_input << std::endl;
        std::cout << "[处理输入] 图片路径: " << image_path << std::endl;
        
        // 如果提供了图片路径，添加到资源记忆中
        if (!image_path.empty()) {
            std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
            std::cout << "[处理输入] 处理图片: " << image_path << std::endl;
            std::cout << "--------------------------------------------------------------------------------" << std::endl;
            add_image_to_memory(image_path);
        }
        
        // 构建上下文
        std::string context = build_context(user_input);
        std::cout << "\n[处理输入] --- 上下文信息 ---\n" << context << "[处理输入] --- 上下文信息结束 ---" << std::endl;
        
        // 保存用户消息到回忆记忆
        RecallMemoryManager recall_manager(storage_);
        RecallMemoryManager::RecallMemoryItem user_item;
        user_item.user_id = user_id_;
        user_item.agent_id = agent_id_; // 使用已初始化的agent ID
        user_item.session_id = session_id_;
        user_item.message_type = "user_message";
        user_item.content = user_input;
        user_item.metadata = "{}";
        
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[处理输入] 保存用户消息到回忆记忆..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        int user_memory_id = -1;
        if (recall_manager.add_recall_memory(user_item)) {
            // 获取刚插入的记录ID
            user_memory_id = sqlite3_last_insert_rowid(storage_.getDatabase());
            std::cout << "[处理输入] 用户消息已保存，ID: " << user_memory_id << std::endl;
            
            // 为用户消息生成并存储embedding
            try {
                VectorSearchManager vectorSearchManager(storage_);
                std::vector<float> user_embedding = vectorSearchManager.generateTextEmbedding(user_input);
                if (!user_embedding.empty()) {
                    if (recall_manager.saveEmbedding(user_memory_id, user_embedding)) {
                        std::cout << "[处理输入] 用户消息embedding已保存" << std::endl;
                    } else {
                        std::cerr << "[处理输入] 保存用户消息embedding失败" << std::endl;
                    }
                } else {
                    std::cerr << "[处理输入] 生成用户消息embedding失败" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[处理输入] 生成/保存用户消息embedding时发生异常: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "[处理输入] 保存用户消息到回忆记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        // 调用模型获取响应
        std::string model_response;
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[处理输入] 调用模型获取响应..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        if (!image_path.empty()) {
            // 多模态处理
            std::cout << "[处理输入] 使用多模态处理..." << std::endl;
            model_response = model_.process_multimodal(user_input, image_path);
        } else {
            // 文本处理
            std::cout << "[处理输入] 使用文本处理..." << std::endl;
            std::vector<std::string> context_lines = {context};
            model_response = model_.generateResponse(context_lines, user_input);
        }
        
        std::cout << "[处理输入] 模型响应: " << model_response << std::endl;
        
        // 保存模型响应到回忆记忆
        RecallMemoryManager::RecallMemoryItem ai_item;
        ai_item.user_id = user_id_;
        ai_item.agent_id = agent_id_; // 使用已初始化的agent ID
        ai_item.session_id = session_id_;
        ai_item.message_type = "ai_response";
        ai_item.content = model_response;
        ai_item.metadata = "{}";
        
        std::cout << "\n--------------------------------------------------------------------------------" << std::endl;
        std::cout << "[处理输入] 保存AI响应到回忆记忆..." << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        int ai_memory_id = -1;
        if (recall_manager.add_recall_memory(ai_item)) {
            // 获取刚插入的记录ID
            ai_memory_id = sqlite3_last_insert_rowid(storage_.getDatabase());
            std::cout << "[处理输入] AI响应已保存，ID: " << ai_memory_id << std::endl;
            
            // 为AI响应生成并存储embedding
            try {
                VectorSearchManager vectorSearchManager(storage_);
                std::vector<float> ai_embedding = vectorSearchManager.generateTextEmbedding(model_response);
                if (!ai_embedding.empty()) {
                    if (recall_manager.saveEmbedding(ai_memory_id, ai_embedding)) {
                        std::cout << "[处理输入] AI响应embedding已保存" << std::endl;
                    } else {
                        std::cerr << "[处理输入] 保存AI响应embedding失败" << std::endl;
                    }
                } else {
                    std::cerr << "[处理输入] 生成AI响应embedding失败" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[处理输入] 生成/保存AI响应embedding时发生异常: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "[处理输入] 保存AI响应到回忆记忆失败: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        }
        
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "[处理输入] 处理用户输入结束" << std::endl;
        std::cout << "================================================================================\n" << std::endl;
        
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

int main() {
    try {
        ChatModelTest chat_test;
        chat_test.run_chat_loop();
        std::cout << "\n测试完成，数据已保存到: test_chat_with_model.db" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}