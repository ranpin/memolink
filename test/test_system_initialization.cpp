#include <iostream>
#include <filesystem>
#include "database/core/ORMWrapper.h"
#include "database/initialization/ConfigManager.h"

int main() {
    std::cout << "=== 系统初始化测试 ===" << std::endl;
    
    // 数据库文件路径
    const std::string dbPath = "../database/system_initialization_test.db";
    
    // 如果数据库文件已存在，先删除它
    if (std::filesystem::exists(dbPath)) {
        std::filesystem::remove(dbPath);
        std::cout << "已删除旧的测试数据库文件" << std::endl;
    }
    
    // 初始化数据库 - 使用ORMWrapper
    ORMWrapper ormWrapper(dbPath);
    // ORM会在构造函数中自动初始化
    
    std::cout << "数据库初始化成功!" << std::endl;
    
    // 测试配置管理器
    std::cout << "配置管理器测试:" << std::endl;
    ConfigManager& config = ConfigManager::getInstance();
    std::cout << "  API密钥: " << (config.getApiKey().empty() ? "未设置" : config.getApiKey()) << std::endl;
    std::cout << "  文本模型名称: " << config.getTextModelName() << std::endl;
    std::cout << "  数据库路径: " << config.getDatabasePath() << std::endl;
    std::cout << "  本地API URL: " << config.getLocalApiUrl() << std::endl;
    std::cout << std::endl;

    std::cout << "=== 系统初始化测试完成 ===" << std::endl;
    
    return 0;
}