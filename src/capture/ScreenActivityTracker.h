#pragma once

#include "../database/core/StorageManager.h"
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

struct ActivityData {
    std::string app_name;
    std::string window_title;
    int duration_seconds;
    std::chrono::system_clock::time_point start_time;
};

class ScreenActivityTracker {
public:
    explicit ScreenActivityTracker(StorageManager& storage);
    ~ScreenActivityTracker();
    
    void start_tracking();
    void stop_tracking();
    bool capture_screenshot(const std::string& file_path);
    bool store_activity_in_memory(int user_id, const ActivityData& activity);
    bool store_screenshot_in_memory(int user_id, const std::string& file_path, const std::string& summary);
    
    // 新增保存截图到资源记忆的方法
    int saveScreenshotToResourceMemory(int user_id, const std::string& file_path, const std::string& summary);
    
    // 新增持续截图功能
    void start_continuous_screenshot(int interval_seconds = 30);
    void stop_continuous_screenshot();

private:
    StorageManager& storage_;
    std::thread tracking_thread_;
    std::thread screenshot_thread_;
    std::atomic<bool> is_tracking_;
    std::atomic<bool> is_screenshot_capturing_;
    
    void track_loop();
    void screenshot_loop(int interval_seconds);
    
    // 添加真实的屏幕截图方法
    bool capture_real_screenshot(const std::string& file_path);
};