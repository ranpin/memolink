#include <iostream>
#include <vector>
#include "../src/memory/context/ContextBuilder.h"
#include "../src/database/managers/MemoryOrchestratorImpl.h"
#include "../src/database/core/StorageManager.h"
#include <iostream>

int main() {
    std::cout << "=== 上下文构建器测试 ===" << std::endl;
    
    // 初始化数据库
    std::string dbPath = "test_context.db";
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return 1;
    }

    // 创建MemoryOrchestrator实例
    MemoryOrchestratorImpl orchestrator(storage.getORMWrapper());

    int user_id = 1;
    std::string query = "人工智能发展历史";

    // 构建上下文
    HierarchicalContext context = ContextBuilder::buildContext(user_id, query, orchestrator);

    // 格式化上下文
    std::string formattedContext = ContextBuilder::formatContext(context);

    std::cout << "Formatted Context:\n" << formattedContext << std::endl;

    if (!formattedContext.empty()) {
        std::cout << "上下文构建成功!" << std::endl;
    } else {
        std::cerr << "上下文构建失败!" << std::endl;
        return 1;
    }

    // 测试空上下文处理
    int non_existent_user_id = 999; // 不存在的用户ID
    std::string test_query = "test query";

    HierarchicalContext emptyContext = ContextBuilder::buildContext(non_existent_user_id, test_query, orchestrator);
    std::string formattedEmptyContext = ContextBuilder::formatContext(emptyContext);

    std::cout << "Empty Context Format:\n" << formattedEmptyContext << std::endl;

    // 即使是空上下文也应该返回有效的格式化字符串
    if (!formattedEmptyContext.empty()) {
        std::cout << "空上下文处理成功!" << std::endl;
    } else {
        std::cerr << "空上下文处理失败!" << std::endl;
        return 1;
    }
    
    std::cout << "=== 上下文构建器测试完成 ===" << std::endl;
    return 0;
}