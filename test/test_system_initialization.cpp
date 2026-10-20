#include <string>
#include <iostream>
#include <filesystem>
#include "database/initialization/InitDB.h"
#include "database/core/StorageManager.h"
#include "database/managers/UserManager.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "database/initialization/ConfigManager/ConfigManager.h"

int main() {
    std::cout << "=== 系统初始化测试 ===" << std::endl;
    
    // 数据库文件路径
    const std::string dbPath = "../database/system_initialization_test.db";
    
    // 如果数据库文件已存在，先删除它
    if (std::filesystem::exists(dbPath)) {
        std::filesystem::remove(dbPath);
        std::cout << "已删除旧的测试数据库文件" << std::endl;
    }
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return 1;
    }
    
    std::cout << "数据库初始化成功!" << std::endl;
    
    // 创建存储管理器
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return 1;
    }
    
    std::cout << "存储管理器初始化成功!" << std::endl;
    
    // 测试配置管理器
    ConfigManager& config = ConfigManager::getInstance();
    std::cout << "配置管理器测试:" << std::endl;
    std::cout << "  API密钥: " << (config.getApiKey().empty() ? "未设置" : "已设置") << std::endl;
    std::cout << "  模型名称: " << config.getTextModelName() << std::endl;
    std::cout << "  数据库路径: " << config.getDatabasePath() << std::endl;
    std::cout << "  本地API URL: " << config.getLocalApiUrl() << std::endl;
    
    // 测试用户管理器
    std::cout << "\n用户管理器测试:" << std::endl;
    int userId = UserManager::getUserIdByName("TestUser", dbPath);
    if (userId <= 0) {
        if (UserManager::addUser("TestUser", "test@example.com", "password123", dbPath)) {
            userId = UserManager::getUserIdByName("TestUser", dbPath);
            std::cout << "  创建用户成功，ID: " << userId << std::endl;
        } else {
            std::cerr << "  创建用户失败!" << std::endl;
            return 1;
        }
    } else {
        std::cout << "  用户已存在，ID: " << userId << std::endl;
    }
    
    // 测试核心记忆管理器
    std::cout << "\n核心记忆管理器测试:" << std::endl;
    CoreMemoryBlockManager coreMemoryManager(storage);
    
    // 添加核心记忆块
    if (coreMemoryManager.addCoreMemoryBlock(userId, "test_key", "test_value", "")) {
        std::cout << "  添加核心记忆块成功" << std::endl;
    } else {
        std::cerr << "  添加核心记忆块失败!" << std::endl;
        return 1;
    }
    
    // 列出核心记忆块
    auto coreMemories = coreMemoryManager.listCoreMemoryBlocks(userId);
    std::cout << "  核心记忆块数量: " << coreMemories.size() << std::endl;
    for (const auto& memory : coreMemories) {
        std::cout << "    " << memory.first << ": " << memory.second << std::endl;
    }
    
    std::cout << "\n=== 系统初始化测试完成 ===" << std::endl;
    
    return 0;
}