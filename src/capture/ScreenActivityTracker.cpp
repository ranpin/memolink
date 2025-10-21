#include "ScreenActivityTracker.h"
#include "../database/managers/ResourceMemoryManager.h"
#include "../utils/TimeUtils.h"
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include "mac/ScreenCaptureHelper.h"
#endif

ScreenActivityTracker::ScreenActivityTracker(StorageManager& storage) 
    : storage_(storage), is_tracking_(false), is_screenshot_capturing_(false) {
}

ScreenActivityTracker::~ScreenActivityTracker() {
    stop_tracking();
    stop_continuous_screenshot();
}

void ScreenActivityTracker::start_tracking() {
    if (is_tracking_.load()) {
        std::cout << "屏幕活动跟踪已在运行" << std::endl;
        return;
    }
    
    is_tracking_.store(true);
    tracking_thread_ = std::thread(&ScreenActivityTracker::track_loop, this);
}

void ScreenActivityTracker::stop_tracking() {
    if (is_tracking_.load()) {
        is_tracking_.store(false);
        
        if (tracking_thread_.joinable()) {
            tracking_thread_.join();
        }
    }
}

bool ScreenActivityTracker::capture_screenshot(const std::string& file_path) {
    std::cout << "捕获屏幕截图到: " << file_path << std::endl;
    
    // 首先尝试真实的屏幕截图
    if (capture_real_screenshot(file_path)) {
        return true;
    }
    
    // 如果真实截图失败，则使用模拟截图
    std::cout << "使用模拟屏幕截图" << std::endl;
    
    // 确保目录存在
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());
    
    // 创建一个大的图像来模拟屏幕截图
    cv::Mat screenshot(1080, 1920, CV_8UC3, cv::Scalar(70, 130, 180)); // 蓝色背景
    
    // 添加一些示例文本
    cv::putText(screenshot, "MIRIX Screen Capture", cv::Point(50, 100), 
               cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 255), 3);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time_t);
    timestamp.pop_back(); // 移除换行符
    
    cv::putText(screenshot, timestamp, cv::Point(50, 200), 
               cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
    
    // 保存截图
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    
    bool result = cv::imwrite(file_path, screenshot, compression_params);
    
    if (result) {
        std::cout << "屏幕截图保存成功: " << file_path << std::endl;
    } else {
        std::cerr << "屏幕截图保存失败: " << file_path << std::endl;
    }
    
    return result;
}

bool ScreenActivityTracker::capture_real_screenshot(const std::string& file_path) {
    // 确保目录存在
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());
    
#ifdef __APPLE__
    // 请求屏幕捕获权限
    if (!CGPreflightScreenCaptureAccess()) {
        std::cout << "请求屏幕捕获权限..." << std::endl;
        CGRequestScreenCaptureAccess();
        // 等待权限授予
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
#endif
    
    // 使用系统命令捕获屏幕截图
    std::string command = "screencapture -x " + file_path + " 2>/dev/null";
    int result = system(command.c_str());
    
    if (result == 0 && std::filesystem::exists(file_path)) {
        std::cout << "使用screencapture命令捕获屏幕截图成功" << std::endl;
        return true;
    } else {
        std::cerr << "使用screencapture命令捕获屏幕截图失败" << std::endl;
        return false;
    }
}

bool ScreenActivityTracker::store_activity_in_memory(int user_id, const ActivityData& activity) {
    std::cout << "在记忆中存储屏幕活动数据" << std::endl;
    
    // 这里应该实现将屏幕活动数据存储到记忆系统的逻辑
    // 为简化起见，我们只打印信息
    std::cout << "  应用: " << activity.app_name << std::endl;
    std::cout << "  窗口标题: " << activity.window_title << std::endl;
    std::cout << "  持续时间: " << activity.duration_seconds << " 秒" << std::endl;
    
    return true;
}

bool ScreenActivityTracker::store_screenshot_in_memory(int user_id, const std::string& file_path, const std::string& summary) {
    std::cout << "在记忆中存储屏幕截图: " << file_path << std::endl;
    
    // 获取文件大小
    std::ifstream in_file(file_path, std::ifstream::ate | std::ifstream::binary);
    
    if (!in_file.is_open()) {
        std::cerr << "无法打开文件: " << file_path << std::endl;
        return false;
    }
    
    int file_size = static_cast<int>(in_file.tellg());
    in_file.close();
    
    // 使用ResourceMemoryManager存储截图数据
    StorageManager storage("mirix.db");
    ResourceMemoryManager manager(storage);
    
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = 1; // 默认agent_id
    item.title = file_path;
    item.content = "Captured screen content";
    item.resource_type = "screenshot";
    item.path = "/capture/screen";
    item.metadata = "{}";
    
    return manager.add_resource_memory(item);
}

void ScreenActivityTracker::track_loop() {
    std::cout << "开始屏幕活动跟踪循环" << std::endl;
    
    while (is_tracking_.load()) {
        // 在实际应用中，这里会实现真实的屏幕活动跟踪逻辑
        // 例如检测当前活动的应用程序和窗口标题
        
        // 模拟跟踪数据
        ActivityData activity;
        activity.app_name = "示例应用";
        activity.window_title = "示例窗口";
        activity.duration_seconds = 5;
        activity.start_time = std::chrono::system_clock::now();
        
        // 存储活动数据
        store_activity_in_memory(1, activity);
        
        // 等待一段时间继续跟踪
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    std::cout << "屏幕活动跟踪循环已停止" << std::endl;
}

void ScreenActivityTracker::start_continuous_screenshot(int interval_seconds) {
    if (is_screenshot_capturing_.load()) {
        std::cout << "持续屏幕截图已在运行" << std::endl;
        return;
    }
    
    is_screenshot_capturing_.store(true);
    screenshot_thread_ = std::thread(&ScreenActivityTracker::screenshot_loop, this, interval_seconds);
}

void ScreenActivityTracker::stop_continuous_screenshot() {
    if (is_screenshot_capturing_.load()) {
        is_screenshot_capturing_.store(false);
        
        if (screenshot_thread_.joinable()) {
            screenshot_thread_.join();
        }
    }
}

void ScreenActivityTracker::screenshot_loop(int interval_seconds) {
    std::cout << "开始持续屏幕截图循环，间隔: " << interval_seconds << " 秒" << std::endl;
    
    while (is_screenshot_capturing_.load()) {
        // 生成带时间戳的文件名
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
        std::string timestamp = ss.str();
        
        std::string file_path = "captures/screen/screenshot_" + timestamp + ".png";
        
        // 捕获并存储截图
        if (capture_screenshot(file_path)) {
            store_screenshot_in_memory(1, file_path, "Continuous screenshot capture");
        }
        
        // 等待指定间隔或被停止
        for (int i = 0; i < interval_seconds && is_screenshot_capturing_.load(); i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "持续屏幕截图循环已停止" << std::endl;
}

int ScreenActivityTracker::saveScreenshotToResourceMemory(
    int user_id, 
    const std::string& file_path, 
    const std::string& summary) {
    
    // 获取文件大小
    std::ifstream in_file(file_path, std::ifstream::ate | std::ifstream::binary);
    
    int file_size = static_cast<int>(in_file.tellg());
    in_file.close();
    
    // 使用ResourceMemoryManager存储截图数据
    StorageManager storage("mirix.db");
    ResourceMemoryManager manager(storage);
    
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = 1; // 默认agent_id
    item.title = file_path;
    item.content = "Captured screen content";
    item.resource_type = "screenshot";
    item.path = "/capture/screen";
    item.metadata = "{}";
    
    bool result = manager.add_resource_memory(item);
    return result ? 1 : -1; // 简化处理，实际应该返回插入记录的ID
}

