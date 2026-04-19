#include "../src/memory/generation/MemoryGenerationManager.h"
#include "../src/database/core/ORMWrapper.h"
#include "../src/database/managers/MemoryOrchestratorImpl.h"
#include <iostream>
#include <memory>

int main() {
    try {
        // 初始化ORMWrapper - 需要提供数据库路径
        ORMWrapper ormWrapper("./test_memory_generation.db");
        
        // 使用MemoryOrchestratorImpl而不是抽象基类
        auto orchestrator = std::make_shared<MemoryOrchestratorImpl>(ormWrapper);
        
        // 创建MemoryGenerationManager实例 - 传入orchestrator参数
        MemoryGenerationManager manager(orchestrator);
        
        std::cout << "Testing Memory Generation Manager..." << std::endl;
        
        // 测试特定用户的消息处理
        int test_user_id = 1;
        if (manager.shouldGenerateMemories(test_user_id)) {
            manager.processUnprocessedMessages(test_user_id);
            std::cout << "Memory generation completed for user " << test_user_id << std::endl;
        } else {
            std::cout << "No conditions met for memory generation for user " << test_user_id << std::endl;
        }
        
        std::cout << "Memory generation test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during memory generation test: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}