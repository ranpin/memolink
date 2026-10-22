#pragma once

// SystemEventManager - 系统事件管理器，用于捕获和处理系统级事件

#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/EpisodicMemoryManager.h"
#include <string>
#include <vector>

// 前向声明capture_system_events函数
std::vector<std::string> capture_system_events();

class SystemEventManager {
private:
    StorageManager& storage_;
    EpisodicMemoryManager* episodicMemoryManager_;

public:
    SystemEventManager(StorageManager& storage);
    
    // 系统事件捕获接口
    bool start_event_capture();
    bool stop_event_capture();
    
    // 事件处理接口
    bool process_system_event(const std::string& event_type, const std::string& event_data);
    
    // 数据存储接口
    bool store_event_data(int user_id, const std::string& event_type, const std::string& event_data);
    
    // 与记忆系统集成的接口
    bool store_event_in_memory(int user_id, const std::string& event_type, const std::string& event_data);
};