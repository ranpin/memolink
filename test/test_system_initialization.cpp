#include <iostream>
#include <string>
#include "database/initialization/InitDB.h"
#include "database/initialization/ConfigManager/ConfigManager.h"
#include "database/core/StorageManager.h"
#include "models/ModelManager.h"
#include "agents/AgentManager.h"
#include "agents/AgentWrapper.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "utils/TimeUtils.h"

int main() {
    std::cout << "=== MIRIX 系统初始化测试 ===" << std::endl;

    // 初始化配置管理器
    ConfigManager& config = ConfigManager::getInstance();
    std::cout << "文本模型名称: " << config.getTextModelName() << std::endl;
    std::cout << "嵌入模型名称: " << config.getEmbeddingModelName() << std::endl;
    std::cout << "多模态模型名称: " << config.getMultimodalModelName() << std::endl;

    // 初始化数据库
    std::string dbPath = "../database/system_initialization_test.db";
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return 1;
    }
    std::cout << "数据库初始化成功: " << dbPath << std::endl;

    // 初始化存储管理器
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return 1;
    }

    // 初始化模型管理器
    ModelManager model(storage);
    std::cout << "模型管理器初始化成功" << std::endl;

    // 初始化代理管理器
    AgentManager agentManager(storage, model);
    std::cout << "代理管理器初始化成功" << std::endl;

    // 创建用户
    int userId = UserManager::addUser("testuser", "test@example.com", "testpassword", dbPath);
    if (userId <= 0) {
        std::cerr << "创建用户失败!" << std::endl;
        return 1;
    }
    std::cout << "用户创建成功，ID: " << userId << std::endl;

    // 创建会话
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = std::to_string(userId);
    sessionItem.token = "test_token";
    sessionItem.start_time = TimeUtils::getCurrentTimestamp();
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.device_info = "test_device";
    sessionItem.metadata = "{}";
    
    if (!sessionManager.create_session(sessionItem)) {
        std::cerr << "创建会话失败!" << std::endl;
        return 1;
    }
    std::cout << "会话创建成功" << std::endl;

    // 添加核心记忆
    CoreMemoryBlockManager coreMemoryManager(storage);
    if (!coreMemoryManager.addCoreMemoryBlock(userId, "test_block", "测试值", "测试核心记忆块")) {
        std::cerr << "添加核心记忆失败!" << std::endl;
        return 1;
    }
    std::cout << "核心记忆添加成功" << std::endl;

    // 创建代理包装器
    AgentWrapper agentWrapper(storage, model);
    // 注意：我们不测试getAgent，因为这需要一个有效的agent ID，而我们没有创建agent
    
    std::cout << "代理包装器初始化成功" << std::endl;

    std::cout << "\n=== 系统初始化测试完成 ===" << std::endl;

    return 0;
}