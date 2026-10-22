#include "legacy/desktop/capture/SystemEventManager.h"
#include "infra/storage/TimeUtils.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>

SystemEventManager::SystemEventManager(StorageManager& storage) 
    : storage_(storage) {
        // EpisodicMemoryManager不需要实例化，使用静态方法
}

bool SystemEventManager::start_event_capture() {
    std::cout << "开始捕获系统事件..." << std::endl;
    // 实际的系统事件捕获启动逻辑应该在这里实现
    return true;
}

bool SystemEventManager::stop_event_capture() {
    std::cout << "停止捕获系统事件..." << std::endl;
    // 实际的系统事件捕获停止逻辑应该在这里实现
    return true;
}

bool SystemEventManager::process_system_event(const std::string& event_type, const std::string& event_data) {
    std::cout << "处理系统事件: " << event_type << std::endl;
    // 实际的系统事件处理逻辑应该在这里实现
    return true;
}

// 将capture_system_events作为独立函数实现
std::vector<std::string> capture_system_events() {
    std::vector<std::string> events;
    
    // 模拟生成随机事件
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    int event_count = dis(gen);
    for (int i = 0; i < event_count; ++i) {
        events.push_back("系统事件 " + std::to_string(i + 1));
    }
    
    return events;
}

bool SystemEventManager::store_event_data(int user_id, const std::string& event_type, const std::string& event_data) {
    std::cout << "存储系统事件数据: " << event_type << std::endl;
    // 实际的系统事件数据存储逻辑应该在这里实现
    return true;
}

bool SystemEventManager::store_event_in_memory(int user_id, const std::string& event_type, const std::string& event_data) {
    std::cout << "在记忆中存储系统事件数据: " << event_type << std::endl;
    
    // 使用EpisodicMemoryManager存储系统事件数据
    return EpisodicMemoryManager::addEpisodicMemory(
        user_id,
        1, // agent_id
        TimeUtils::getCurrentTimestamp(),
        "System Event: " + event_type,
        event_data,
        "system_event",
        "/system/events/" + event_type,
        "{\"event_type\": \"" + event_type + "\"}",
        "mirix.db"
    );
}