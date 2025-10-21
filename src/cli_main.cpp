#include "cli/CommandLineInterface.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/initialization/ConfigManager/ConfigManager.h"
#include <iostream>
#include <memory>

int main() {
    try {
        // 初始化配置管理器（使用单例模式）
        ConfigManager& config = ConfigManager::getInstance();
        
        // 初始化数据库
        std::string dbPath = config.getDatabasePath();
        StorageManager storage(dbPath);
        
        if (!InitDB::initializeDatabase(dbPath)) {
            std::cerr << "数据库初始化失败" << std::endl;
            return 1;
        }
        
        // 创建并启动命令行界面
        CommandLineInterface cli(storage);
        cli.start();
        
    } catch (const std::exception& e) {
        std::cerr << "系统错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}