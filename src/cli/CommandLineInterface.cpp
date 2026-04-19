#include "CommandLineInterface.h"
#include "../database/initialization/ConfigManager.h"
#include "../database/core/StorageManager.h"
#include "../database/core/ORMWrapper.h"
#include "../database/managers/UserManager.h"
#include "../database/managers/SessionManager.h"
#include "../memory/generation/MemoryGenerationManager.h"
#include "../memory/scheduling/MemoryGenerationScheduler.h"
#include "../database/managers/MemoryOrchestratorImpl.h"
#include "../database/models/MemoryModels.h"
#include "../models/ModelManager.h"
#include "../models/EmbeddingIndexManager.h"
#include "../capture/ScreenActivityTracker.h"
#include "../capture/CameraCaptureManager.h"
#include "../capture/AudioCaptureManager.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace std;

CommandLineInterface::CommandLineInterface(StorageManager& storage) 
    : storage_(storage), 
      model_manager_(storage),
      chat_agent_(nullptr),
      memory_agent_(nullptr),
      initialized_(false) {
}

CommandLineInterface::~CommandLineInterface() = default;

void CommandLineInterface::initialize() {
    cout << "MIRIX CLI初始化中..." << endl;
    
    // 初始化代理
    // agent_manager_ = make_unique<AgentManager>(storage_);
    // chat_agent_ = make_unique<ChatAgent>();
    // memory_agent_ = make_unique<MemoryAgent>();
    
    initialized_ = true;
    cout << "MIRIX CLI初始化完成!" << endl;
}

void CommandLineInterface::run() {
    if (!initialized_) {
        cerr << "CLI未初始化，请先调用initialize()" << endl;
        return;
    }
    
    cout << "MIRIX CLI启动中..." << endl;
    cout << "提示：输入 'help' 查看命令，'memory clear-all' 可清空本地演示数据。" << endl;
    
    // 主循环
    string input;
    while (true) {
        cout << "\nMIRIX> ";
        getline(cin, input);
        
        if (input == "exit" || input == "quit") {
            break;
        }
        
        handle_user_input(input);
    }
    
    cout << "再见!" << endl;
}

void CommandLineInterface::show_help() {
    cout << "可用命令:" << endl;
    cout << "  help - 显示此帮助信息" << endl;
    cout << "  chat <消息> - 与AI聊天（当前建议通过demo程序体验）" << endl;
    cout << "  context show - 构建并显示当前对话上下文（调试用）" << endl;
    cout << "  memory list [type] - 列出记忆，type 可为 message/episodic/semantic/core/procedural/multimodal/all" << endl;
    cout << "  memory generate - 生成新记忆（使用配置阈值）" << endl;
    cout << "  memory generate-now - 立即触发后台记忆生成调度" << endl;
    cout << "  memory check-index - 检查 embedding 索引一致性" << endl;
    cout << "  memory clean-orphans - 清理孤儿 embedding（需先执行 check-index）" << endl;
    cout << "  memory clear-all - 清空记忆与多模态数据（用于隐私清理）" << endl;
    cout << "  exit/quit - 退出程序" << endl;
}

void CommandLineInterface::handle_user_input(const std::string& input) {
    if (input == "help") {
        show_help();
        return;
    }
    
    if (input == "context show") {
        show_context();
        return;
    }
    
    if (input.rfind("chat", 0) == 0) {
        cout << "chat 命令暂未实现，请使用现有 demo 或测试程序进行对话体验." << endl;
        return;
    }
    
    if (input.rfind("memory list", 0) == 0) {
        std::string type = "all";
        const std::string prefix = "memory list";
        if (input.size() > prefix.size()) {
            type = input.substr(prefix.size());
            auto pos = type.find_first_not_of(" \t");
            if (pos != std::string::npos) {
                type = type.substr(pos);
            } else {
                type = "all";
            }
        }
        list_memories(type);
        return;
    }
    
    if (input == "memory clear-all") {
        clear_all_memories();
        return;
    }
    
    if (input == "memory check-index") {
        check_index_consistency();
        return;
    }
    
    if (input == "memory clean-orphans") {
        clean_orphaned_embeddings();
        return;
    }
    
    if (input == "memory generate" || input == "memory generate-now") {
        try {
            ORMWrapper& orm = storage_.getORMWrapper();
            MemoryOrchestratorImpl orchestrator_impl(orm);
            MemoryOrchestrator& orchestrator = orchestrator_impl;
            cout << "[CLI] 手动触发记忆生成调度..." << endl;
            MemoryGenerationScheduler::periodicCheck(orchestrator);
            cout << "[CLI] 记忆生成调度完成（如满足条件，将生成长期记忆与向量索引）。" << endl;
        } catch (const std::exception& e) {
            cerr << "[CLI] 触发记忆生成失败: " << e.what() << endl;
        }
        return;
    }
    
    cout << "未知命令. 输入 'help' 查看帮助." << endl;
}

void CommandLineInterface::show_context() {
    try {
        ORMWrapper& orm = storage_.getORMWrapper();
        auto orchestrator = std::make_shared<MemoryOrchestratorImpl>(orm);
        ChatAgent chat_agent(orchestrator);
        int user_id = 1;
        int agent_id = 1;
        int session_id = 1;
        chat_agent.initialize(user_id, agent_id);
        std::string context = chat_agent.buildContext(user_id, agent_id, session_id, "用于调试的查询");
        std::cout << "\n===== 当前对话上下文 =====" << std::endl;
        std::cout << context << std::endl;
        std::cout << "===== 上下文结束 =====" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[CLI] 构建上下文失败: " << e.what() << std::endl;
    }
}

void CommandLineInterface::list_memories(const std::string& type) {
    try {
        ORMWrapper& orm = storage_.getORMWrapper();
        std::string t = type;
        if (t.empty()) {
            t = "all";
        }
        std::cout << "[CLI] 列出记忆，类型: " << t << std::endl;

        auto print_separator = []() {
            std::cout << "----------------------------------------" << std::endl;
        };

        const int max_items = 10;

        if (t == "all" || t == "message") {
            auto messages = orm.getAll<MessageMemory>();
            std::cout << "消息记忆（共 " << messages.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = messages.rbegin(); it != messages.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id
                          << ", session_id=" << it->session_id << std::endl;
                std::cout << "  内容前80字: " << it->content.substr(0, 80) << std::endl;
            }
            print_separator();
        }

        if (t == "all" || t == "episodic") {
            auto episodics = orm.getAll<EpisodicMemory>();
            std::cout << "情景记忆（共 " << episodics.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = episodics.rbegin(); it != episodics.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id << std::endl;
                std::cout << "  事件: " << it->event_description.substr(0, 80) << std::endl;
            }
            print_separator();
        }

        if (t == "all" || t == "semantic") {
            auto semantics = orm.getAll<SemanticMemory>();
            std::cout << "语义记忆（共 " << semantics.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = semantics.rbegin(); it != semantics.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id << std::endl;
                std::cout << "  事实: " << it->fact.substr(0, 80) << std::endl;
            }
            print_separator();
        }

        if (t == "all" || t == "core") {
            auto cores = orm.getAll<CoreMemory>();
            std::cout << "核心记忆（共 " << cores.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = cores.rbegin(); it != cores.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id << std::endl;
                std::cout << "  内容: " << it->content.substr(0, 80) << std::endl;
            }
            print_separator();
        }

        if (t == "all" || t == "procedural") {
            auto procs = orm.getAll<ProceduralMemory>();
            std::cout << "程序记忆（共 " << procs.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = procs.rbegin(); it != procs.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id << std::endl;
                std::cout << "  步骤: " << it->description.substr(0, 80) << std::endl;
            }
            print_separator();
        }

        if (t == "all" || t == "multimodal") {
            auto multimodals = orm.getAll<MultimodalMemory>();
            std::cout << "多模态记忆（共 " << multimodals.size() << " 条）:" << std::endl;
            print_separator();
            int count = 0;
            for (auto it = multimodals.rbegin(); it != multimodals.rend() && count < max_items; ++it, ++count) {
                std::cout << "ID=" << it->id << ", user_id=" << it->user_id
                          << ", 类型=" << it->data_type << std::endl;
                std::cout << "  描述: " << it->description.substr(0, 80) << std::endl;
                std::cout << "  路径: " << it->file_path << std::endl;
            }
            print_separator();
        }
    } catch (const std::exception& e) {
        std::cerr << "[CLI] 列出记忆失败: " << e.what() << std::endl;
    }
}

void CommandLineInterface::clear_all_memories() {
    try {
        ORMWrapper& orm = storage_.getORMWrapper();
        std::cout << "[CLI] 即将清空消息、长期记忆、多模态数据与溯源记录..." << std::endl;

        auto messages = orm.getAll<MessageMemory>();
        for (const auto& m : messages) {
            orm.remove<MessageMemory>(m.id);
        }
        std::cout << "  已清空 message_memories，删除记录数: " << messages.size() << std::endl;

        auto episodics = orm.getAll<EpisodicMemory>();
        for (const auto& em : episodics) {
            orm.remove<EpisodicMemory>(em.id);
        }
        std::cout << "  已清空 episodic_memories，删除记录数: " << episodics.size() << std::endl;

        auto semantics = orm.getAll<SemanticMemory>();
        for (const auto& sm : semantics) {
            orm.remove<SemanticMemory>(sm.id);
        }
        std::cout << "  已清空 semantic_memories，删除记录数: " << semantics.size() << std::endl;

        auto cores = orm.getAll<CoreMemory>();
        for (const auto& cm : cores) {
            orm.remove<CoreMemory>(cm.id);
        }
        std::cout << "  已清空 core_memories，删除记录数: " << cores.size() << std::endl;

        auto procs = orm.getAll<ProceduralMemory>();
        for (const auto& pm : procs) {
            orm.remove<ProceduralMemory>(pm.id);
        }
        std::cout << "  已清空 procedural_memories，删除记录数: " << procs.size() << std::endl;

        auto secrets = orm.getAll<SecretVault>();
        for (const auto& sv : secrets) {
            orm.remove<SecretVault>(sv.id);
        }
        std::cout << "  已清空 secret_vault，删除记录数: " << secrets.size() << std::endl;

        auto multimodals = orm.getAll<MultimodalMemory>();
        for (const auto& mm : multimodals) {
            orm.remove<MultimodalMemory>(mm.id);
        }
        std::cout << "  已清空 multimodal_memories，删除记录数: " << multimodals.size() << std::endl;

        auto provenances = orm.getAll<MemoryProvenance>();
        for (const auto& mp : provenances) {
            orm.remove<MemoryProvenance>(mp.id);
        }
        std::cout << "  已清空 memory_provenance，删除记录数: " << provenances.size() << std::endl;

        // 清空 embedding_index 表（级联删除）
        std::string db_path = storage_.getDatabasePath();
        sqlite3* db = nullptr;
        int rc = sqlite3_open(db_path.c_str(), &db);
        if (rc == SQLITE_OK) {
            const char* deleteSQL = "DELETE FROM embedding_index";
            char* errMsg = nullptr;
            rc = sqlite3_exec(db, deleteSQL, nullptr, nullptr, &errMsg);
            if (rc == SQLITE_OK) {
                int deleted_count = sqlite3_changes(db);
                std::cout << "  已清空 embedding_index，删除记录数: " << deleted_count << std::endl;
            } else {
                std::cerr << "  警告: 清空 embedding_index 失败: " << errMsg << std::endl;
                sqlite3_free(errMsg);
            }
            sqlite3_close(db);
        } else {
            std::cerr << "  警告: 无法打开数据库以清空 embedding_index" << std::endl;
        }

        // 清理captures目录下的多模态文件（仅限本项目目录）
        std::error_code ec;
        std::filesystem::remove_all("../captures", ec);
        std::filesystem::create_directories("../captures/camera", ec);
        std::filesystem::create_directories("../captures/audio", ec);
        std::filesystem::create_directories("../captures/screen", ec);

        std::cout << "[CLI] 隐私清理完成（数据库与本地captures目录）" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[CLI] 清空记忆失败: " << e.what() << std::endl;
    }
}

void CommandLineInterface::check_index_consistency() {
    try {
        std::cout << "\n[CLI] 开始检查 embedding 索引一致性..." << std::endl;
        
        EmbeddingIndexManager indexManager;
        int user_id = 1; // 默认检查用户 1
        std::string dbPath = "../database/mirix.db";
        
        auto report = indexManager.checkIndexConsistency(user_id, dbPath);
        
        // 保存孤儿 ID 以便后续清理
        last_orphaned_ids_ = report.orphaned_ids;
        
        std::cout << "\n[CLI] ========== 检查结果总结 ==========" << std::endl;
        std::cout << "[CLI] 总 embedding 数  : " << report.total_embeddings << std::endl;
        std::cout << "[CLI] 有效 embedding   : " << report.valid_embeddings << std::endl;
        std::cout << "[CLI] 孤儿 embedding   : " << report.orphaned_embeddings << std::endl;
        std::cout << "[CLI] user_id 不匹配: " << report.user_mismatch << std::endl;
        
        if (report.orphaned_embeddings > 0) {
            std::cout << "\n[CLI] 提示: 检测到 " << report.orphaned_embeddings << " 个孤儿 embedding" << std::endl;
            std::cout << "[CLI] 执行 'memory clean-orphans' 可以清理它们\n" << std::endl;
        } else {
            std::cout << "\n[CLI] 索引一致性良好，无需清理\n" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[CLI] 检查索引一致性失败: " << e.what() << std::endl;
    }
}

void CommandLineInterface::clean_orphaned_embeddings() {
    if (last_orphaned_ids_.empty()) {
        std::cout << "[CLI] 没有待清理的孤儿 embedding" << std::endl;
        std::cout << "[CLI] 请先执行 'memory check-index' 检查索引一致性" << std::endl;
        return;
    }
    
    try {
        std::cout << "\n[CLI] 确认清理 " << last_orphaned_ids_.size() << " 个孤儿 embedding? (y/n): ";
        std::string confirm;
        std::getline(std::cin, confirm);
        
        if (confirm != "y" && confirm != "Y") {
            std::cout << "[CLI] 已取消清理\n" << std::endl;
            return;
        }
        
        EmbeddingIndexManager indexManager;
        std::string dbPath = "../database/mirix.db";
        
        bool success = indexManager.cleanOrphanedEmbeddings(last_orphaned_ids_, dbPath);
        
        if (success) {
            std::cout << "[CLI] 清理完成，建议重新执行 'memory check-index' 验证\n" << std::endl;
            last_orphaned_ids_.clear();
        } else {
            std::cerr << "[CLI] 清理失败\n" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[CLI] 清理孤儿 embedding 失败: " << e.what() << std::endl;
    }
}