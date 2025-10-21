#include <iostream>
#include <string>
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/RecallMemoryManager.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "database/managers/EpisodicMemoryManager.h"
#include "database/managers/SemanticMemoryManager.h"
#include "database/managers/ProceduralMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/managers/KnowledgeVaultManager.h"
#include "database/managers/ArchivalMemoryManager.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/AuditLogManager.h"
#include "database/managers/BackupManager.h"
#include "database/managers/MemoryOrchestrator.h"
#include "database/initialization/ConfigManager/ConfigManager.h"
#include "models/ModelManager.h"
#include "models/EmbeddingIndexManager.h"
#include "agents/AgentWrapper.h"
#include "agents/AgentManager.h"
#include "agents/ChatAgent.h"
#include "agents/CoreMemoryAgent.h"
#include "agents/EpisodicMemoryAgent.h"
#include "agents/SemanticMemoryAgent.h"
#include "agents/ProceduralMemoryAgent.h"
#include "agents/ResourceMemoryAgent.h"
#include "agents/KnowledgeVaultAgent.h"

using namespace std;

void testLongTermMemory() {
    cout << "=== 测试长期记忆 ===" << endl;
    
    string dbPath = "test_long_term_memory.db";
    StorageManager storage(dbPath);
    
    if (!storage.initialize()) {
        cerr << "数据库初始化失败!" << endl;
        return;
    }
    
    if (!InitDB::initializeDatabase(dbPath)) {
        cerr << "数据库表初始化失败!" << endl;
        return;
    }
    
    // 添加用户
    bool userAdded = UserManager::addUser("testuser", "test@example.com", "", dbPath);
    if (!userAdded) {
        std::cerr << "Failed to add user" << std::endl;
        return;
    }
    std::cout << "User added successfully" << std::endl;
    
    // 获取用户ID
    int userId = UserManager::getUserIdByName("testuser", dbPath);
    if (userId <= 0) {
        std::cerr << "Failed to get user ID" << std::endl;
        return;
    }

    // 测试会话管理器
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = to_string(userId);
    sessionItem.start_time = "2023-01-01 00:00:00";
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.metadata = "{}";
    
    bool sessionCreated = sessionManager.create_session(sessionItem);
    int sessionId = 1;
    if (!sessionCreated) {
        cerr << "创建会话失败!" << endl;
        return;
    }
    
    // 测试模型管理器
    ModelManager modelManager(storage);
    
    // 测试代理管理器
    AgentManager agentManager(storage, modelManager);
    AgentWrapper* agentWrapper = agentManager.get_agent_wrapper();
    if (agentWrapper) {
        cout << "代理管理器初始化成功" << endl;
    } else {
        cerr << "代理管理器初始化失败!" << endl;
    }
    
    cout << "长期记忆测试完成" << endl;
}

int main() {
    cout << "开始测试长期记忆..." << endl;
    
    testLongTermMemory();
    
    cout << "长期记忆测试完成!" << endl;
    return 0;
}
