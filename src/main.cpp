#include "cli/CommandLineInterface.h"
#include "database/core/StorageManager.h"
#include <iostream>

int main() {
    try {
        // 初始化存储管理器
        StorageManager storage("mirix.db");
        
        // 创建并运行命令行界面
        CommandLineInterface cli(storage);
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}