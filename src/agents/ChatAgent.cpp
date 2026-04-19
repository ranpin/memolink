#include "ChatAgent.h"
#include "../models/QwenAPI.h"
#include "../models/ModelManager.h"
#include "../models/VectorSearchManager.h"
#include "../database/core/StorageManager.h"
#include "../database/initialization/ConfigManager.h"
#include "../database/models/MemoryModels.h"
#include "../database/managers/MessageMemoryManager.h"
#include "../memory/context/ContextBuilder.h"
#include "../memory/generation/MemoryGenerationManager.h"
#include "../memory/scheduling/MemoryGenerationScheduler.h"
#include "../utils/TimeUtils.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <json/json.h>

ChatAgent::ChatAgent(std::shared_ptr<MemoryOrchestrator> orchestrator) 
    : orchestrator_(orchestrator), storage_manager_(nullptr), vector_search_manager_(nullptr) {
    
    // 初始化 VectorSearchManager
    try {
        ConfigManager& config_manager = ConfigManager::getInstance();
        std::string db_path = config_manager.getDatabasePath();
        if (db_path.empty()) {
            db_path = "mirix.db";
        }
        
        // 创建 StorageManager 并持有其所有权
        storage_manager_ = std::make_unique<StorageManager>(db_path);
        if (storage_manager_->initialize()) {
            vector_search_manager_ = std::make_unique<VectorSearchManager>(*storage_manager_);
            std::cout << "[ChatAgent] VectorSearchManager 初始化成功" << std::endl;
        } else {
            std::cerr << "[ChatAgent] StorageManager 初始化失败，VectorSearchManager 将不可用" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ChatAgent] VectorSearchManager 初始化失败: " << e.what() << std::endl;
    }
}

void ChatAgent::initialize(int userId, int agentId) {
    user_id_ = userId;
    agent_id_ = agentId;
}

std::string ChatAgent::buildContext(int user_id, int agent_id, int session_id, const std::string& message) {
    // 使用 ContextBuilder 构建分层上下文（最近消息 + 各类长期记忆 + 资源记忆）
    std::cout << "[ChatAgent] 构建上下文..." << std::endl;
    
    ContextBuilder builder;
    
    // 如果 VectorSearchManager 可用，使用向量检索
    VectorSearchManager* vector_search = vector_search_manager_ ? vector_search_manager_.get() : nullptr;
    
    if (vector_search) {
        std::cout << "[ChatAgent] 使用向量检索构建上下文" << std::endl;
    } else {
        std::cout << "[ChatAgent] 使用传统方法构建上下文" << std::endl;
    }
    
    HierarchicalContext hierarchicalContext = builder.buildContext(user_id, message, *orchestrator_, vector_search, session_id);
    std::string context = ContextBuilder::formatContext(hierarchicalContext, 0); //原先是2000，进行了截断，设为表示不截断。
    
    std::cout << "[ChatAgent] 上下文构建完成，长度: " << context.length() << " 字符" << std::endl;
    
    // 打印最终上下文内容（截断版，便于观测）
    // std::cout << "\n[ChatAgent] ========== 最终上下文预览 ==========" << std::endl;
    // if (context.length() <= 800) {
    //     std::cout << context << std::endl;
    // } else {
    //     std::cout << context.substr(0, 800) << "\n... (剩余 " 
    //               << (context.length() - 800) << " 字符)" << std::endl;
    // }
    // std::cout << "[ChatAgent] ========================================\n" << std::endl;

    std::cout << "\n[ChatAgent] ========== 最终上下文预览 ==========" << std::endl;
    std::cout << context << std::endl;

    std::cout << "[ChatAgent] ========================================\n" << std::endl; 
    
    // 导出完整上下文到文件（用于详细分析）
    try {
        // 使用绝对路径或相对于工作目录的正确路径
        ConfigManager& cfg = ConfigManager::getInstance();

        std::filesystem::path db = std::filesystem::absolute(cfg.getDatabasePath()); // 关键：绝对化
        std::filesystem::path logs_dir = db.parent_path().parent_path() / "logs";    // .../mirix-cpp/logs
        std::filesystem::create_directories(logs_dir);

        std::string ts = TimeUtils::getCurrentTimestamp();
        for (char& c : ts) if (c == ':' || c == ' ') c = '_'; // 防止文件名怪

        std::filesystem::path context_file =
            logs_dir / ("context_session_" + std::to_string(session_id) + "_" + ts + ".txt");

        std::ofstream out(context_file.string());
        if (out.is_open()) {
            out << "========== User Query ==========\n";
            out << message << "\n\n";
            out << "========== Full Context ==========\n";
            out << context << "\n";
            out.close();
            std::cout << "[ChatAgent] 上下文已导出到: " << context_file << std::endl;
        } else {
            std::cerr << "[ChatAgent] ⚠ 无法创建上下文文件: " << context_file << std::endl;
        }
    } catch (const std::exception& e) {
        // 静默失败，不影响主流程
        std::cerr << "[ChatAgent] 上下文导出异常: " << e.what() << std::endl;
    }
    
    return context;
}

std::string ChatAgent::chat(int user_id, int agent_id, int session_id, const std::string& message) {
    auto start = std::chrono::steady_clock::now();
    // 构建上下文（使用 ContextBuilder 聚合多种记忆）
    std::string context = buildContext(user_id, agent_id, session_id, message);
    
    // 使用 ModelManager + StorageManager 调用实际大模型（与 ChatModelTest 保持一致）
    ConfigManager& config_manager = ConfigManager::getInstance();
    std::string db_path = config_manager.getDatabasePath();
    if (db_path.empty()) {
        db_path = "mirix.db"; // 默认值，保持与其他组件一致
    }
    
    std::string response;
    try {
        StorageManager storage(db_path);
        if (!storage.initialize()) {
            std::cerr << "[ChatAgent] StorageManager 初始化失败，使用占位响应" << std::endl;
            response = "Storage 初始化失败，无法调用模型。";
        } else {
            ModelManager model_manager(storage);
            std::vector<std::string> context_lines = {context};
            response = model_manager.generateResponse(context_lines, message);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ChatAgent] 调用模型失败: " << e.what() << std::endl;
        response = std::string("调用模型失败: ") + e.what();
    }
    
    // 获取MessageMemoryManager
    MessageMemoryManager& message_memory_manager = orchestrator_->getMessageMemoryManager();
    
    // 保存用户输入到MessageMemory
    MessageMemory user_memory;
    user_memory.user_id = user_id;  // 用户ID始终是真实用户
    user_memory.agent_id = agent_id;  // AI Agent ID
    user_memory.session_id = session_id;
    user_memory.content = "[用户] " + message;  // 用前缀区分角色
    user_memory.created_at = TimeUtils::getCurrentTimestamp();
    message_memory_manager.add(user_memory);
    
    // 保存AI响应到MessageMemory
    MessageMemory ai_memory;
    ai_memory.user_id = user_id;  // user_id 始终是真实用户ID，不交换
    ai_memory.agent_id = agent_id;  // AI Agent ID
    ai_memory.session_id = session_id;
    ai_memory.content = "[AI] " + response;  // 用前缀区分角色
    ai_memory.created_at = TimeUtils::getCurrentTimestamp();
    message_memory_manager.add(ai_memory);
    
    // 异步触发后台记忆生成调度器，不阻塞当前对话
    try {
        auto orchestrator_copy = orchestrator_;
        std::thread scheduler_thread([
            orchestrator_copy
        ]() {
            try {
                if (orchestrator_copy) {
                    MemoryGenerationScheduler::periodicCheck(*orchestrator_copy);
                }
            } catch (const std::exception& e) {
                std::cerr << "[ChatAgent] MemoryGenerationScheduler 异步检查失败: " << e.what() << std::endl;
            }
        });
        scheduler_thread.detach();
    } catch (const std::exception& e) {
        std::cerr << "[ChatAgent] 启动异步记忆调度线程失败: " << e.what() << std::endl;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[Perf] ChatAgent::chat total latency: " << elapsed_ms
              << " ms, context length=" << context.length() << std::endl;
    
    return response;
}