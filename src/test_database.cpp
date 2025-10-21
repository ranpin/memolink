#include <iostream>
#include <string>
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"

void testDatabaseInitialization() {
    std::cout << "=== 测试数据库初始化 ===" << std::endl;
    
    std::string dbPath = "test_database.db";
    
    // 测试数据库初始化
    if (InitDB::initializeDatabase(dbPath)) {
        std::cout << "数据库初始化成功: " << dbPath << std::endl;
        
        // 测试数据库连接
        StorageManager storage(dbPath);
        if (storage.initialize()) {
            std::cout << "数据库连接成功" << std::endl;
        } else {
            std::cerr << "数据库连接失败!" << std::endl;
        }
    } else {
        std::cerr << "数据库初始化失败!" << std::endl;
    }
    
    std::cout << "数据库初始化测试完成" << std::endl;
}

int main() {
    std::cout << "开始测试数据库功能..." << std::endl;
    
    testDatabaseInitialization();
    
    std::cout << "数据库测试完成!" << std::endl;
    return 0;
}