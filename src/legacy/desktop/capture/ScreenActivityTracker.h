#pragma once

#include "infra/storage/StorageManager.h"
#include "legacy/infra-alt/ModelManager.h"
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
private:
    StorageManager& storage_;
    std::atomic<bool> is_tracking_;
    std::atomic<bool> is_screenshot_capturing_;
    std::thread tracking_thread_;
    std::thread screenshot_thread_;
    ModelManager* modelManager_;
    bool verbose_output_;

    void compress_image(const std::string& file_path, const std::string& output_path);

public:
    // 构造函数，接受StorageManager引用
    explicit ScreenActivityTracker(StorageManager& storage);

    // 析构函数
    ~ScreenActivityTracker();

    // 设置ModelManager引用
    void setModelManager(ModelManager* modelManager);

    // 设置详细输出模式
    void setVerboseOutput(bool verbose);

    // 存储屏幕活动数据到记忆中
    bool store_activity_in_memory(int user_id, const ActivityData& activity);

    // 开始和停止屏幕活动跟踪
    void start_tracking();
    void stop_tracking();

    // 开始和停止持续屏幕截图
    void start_continuous_screenshot(int interval_seconds);
    void stop_continuous_screenshot();

    // 捕获屏幕截图
    bool capture_screenshot(const std::string& file_path);

    // 保存截图到资源记忆（已废弃）
    int saveScreenshotToResourceMemory(int user_id, const std::string& file_path, const std::string& summary);

    // 存储屏幕截图到资源记忆中（新方法，推荐使用）
    bool store_screenshot_in_memory(int user_id, const std::string& file_path, const std::string& summary);

private:
    void track_loop();
    void screenshot_loop(int interval_seconds);
    bool capture_real_screenshot(const std::string& file_path);
};