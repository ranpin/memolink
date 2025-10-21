#include <iostream>
#include <string>
#include <vector>
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "capture/ScreenActivityTracker.h"
#include "capture/CameraCaptureManager.h"
#include "capture/AudioCaptureManager.h"
#include "capture/SystemEventManager.h"
#include "models/ModelManager.h"

void testContentMetadata() {
    std::cout << "=== 测试内容元数据处理 ===" << std::endl;
    
    std::string dbPath = "test_content_metadata.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    // 添加测试用户
    bool userAdded = UserManager::addUser("testuser", "test@example.com", "password_hash", dbPath);
    if (!userAdded) {
        std::cerr << "添加用户失败!" << std::endl;
        return;
    }
    
    int userId = UserManager::getUserIdByName("testuser", dbPath);
    if (userId <= 0) {
        std::cerr << "获取用户ID失败!" << std::endl;
        return;
    }
    
    std::cout << "用户添加成功，用户ID: " << userId << std::endl;
    
    // 初始化存储管理器
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    // 创建会话
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = std::to_string(userId);
    sessionItem.start_time = "2023-01-01 00:00:00";
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.metadata = "{}";
    
    bool sessionCreated = sessionManager.create_session(sessionItem);
    if (!sessionCreated) {
        std::cerr << "创建会话失败!" << std::endl;
        return;
    }
    
    std::cout << "会话创建成功" << std::endl;
    
    // 测试摄像头捕获元数据
    std::cout << "\n1. 测试摄像头捕获元数据:" << std::endl;
    CameraCaptureManager cameraManager(storage);
    
    // 测试屏幕活动跟踪器
    std::cout << "\n2. 测试屏幕活动跟踪器:" << std::endl;
    ScreenActivityTracker screenTracker(storage);
    
    // 测试音频捕获管理器
    std::cout << "\n3. 测试音频捕获管理器:" << std::endl;
    AudioCaptureManager audioManager(storage);
    
    // 测试系统事件管理器
    std::cout << "\n4. 测试系统事件管理器:" << std::endl;
    SystemEventManager eventManager(storage);
    
    // 测试资源记忆管理器
    std::cout << "\n5. 测试资源记忆管理器:" << std::endl;
    bool resourceAdded = ResourceMemoryManager::addResourceMemory(
        userId, 
        1, 
        "test_resource.txt", 
        "text", 
        "测试资源", 
        "这是测试资源的内容", 
        100, 
        "/test", 
        "{}",
        dbPath
    );
    
    if (resourceAdded) {
        std::cout << "资源记忆添加成功" << std::endl;
        
        // 查询资源记忆
        std::vector<ResourceMemoryRecord> resources = ResourceMemoryManager::getAllResourceMemories(userId, dbPath);
        std::cout << "查询到 " << resources.size() << " 个资源记忆" << std::endl;
        
        if (!resources.empty()) {
            ResourceMemoryRecord record = resources[0];
            std::cout << "  资源ID: " << record.id << std::endl;
            std::cout << "  文件名: " << record.filename << std::endl;
            std::cout << "  资源类型: " << record.resource_type << std::endl;
            std::cout << "  摘要: " << record.summary << std::endl;
        }
    } else {
        std::cerr << "资源记忆添加失败!" << std::endl;
    }
    
    std::cout << "\n内容元数据处理测试完成" << std::endl;
}

int main() {
    std::cout << "开始测试内容元数据处理..." << std::endl;
    
    testContentMetadata();
    
    std::cout << "内容元数据处理测试完成!" << std::endl;
    return 0;
}