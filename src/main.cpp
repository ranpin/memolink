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

int main() {
    std::cout << "=== MIRIX 智能记忆系统 ===" << std::endl;

    // 初始化配置管理器
    ConfigManager& config = ConfigManager::getInstance();
    std::cout << "文本模型名称: " << config.getTextModelName() << std::endl;
    std::cout << "嵌入模型名称: " << config.getEmbeddingModelName() << std::endl;
    std::cout << "多模态模型名称: " << config.getMultimodalModelName() << std::endl;

    // 初始化数据库
    std::string dbPath = "mirix.db";
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
    std::string passwordHash = UserManager::hashPassword("testpassword");
    if (!UserManager::addUser("testuser", "test@example.com", passwordHash, dbPath)) {
        std::cerr << "创建用户失败!" << std::endl;
        return 1;
    }
    std::cout << "用户创建成功" << std::endl;

    // 创建会话
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = "1";
    sessionItem.start_time = "2023-01-01 00:00:00";
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.metadata = "{}";
    
    if (!sessionManager.create_session(sessionItem)) {
        std::cerr << "创建会话失败!" << std::endl;
        return 1;
    }
    std::cout << "会话创建成功" << std::endl;

    // 测试核心记忆
    CoreMemoryBlockManager coreMemoryManager(storage);
    if (!coreMemoryManager.addCoreMemoryBlock(1, "test_key", "test_value", "Test core memory")) {
        std::cerr << "添加核心记忆失败!" << std::endl;
        return 1;
    }
    std::cout << "核心记忆添加成功" << std::endl;

    // 测试代理包装器
    AgentWrapper* agentWrapper = agentManager.get_agent_wrapper();
    if (!agentWrapper) {
        std::cerr << "获取代理包装器失败!" << std::endl;
        return 1;
    }
    std::cout << "代理包装器获取成功" << std::endl;

    std::cout << "\n=== 系统初始化完成 ===" << std::endl;
    return 0;
}