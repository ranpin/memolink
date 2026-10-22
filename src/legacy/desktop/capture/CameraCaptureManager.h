#pragma once

#include "infra/storage/StorageManager.h"
#include <string>
#include <thread>
#include <atomic>

class CameraCaptureManager {
public:
    explicit CameraCaptureManager(StorageManager& storage);
    ~CameraCaptureManager();
    
    bool capture_image(const std::string& file_path);
    bool start_video_capture(const std::string& file_path);
    void stop_capture();
    
    bool store_capture_in_memory(int user_id, 
                                const std::string& file_path,
                                const std::string& capture_type,
                                const std::string& summary);
    
    // 新增持续捕获功能
    void start_continuous_capture(int interval_seconds = 30);
    void stop_continuous_capture();
    
    // 新增保存到资源记忆的方法
    int saveCaptureToResourceMemory(int user_id, 
                                   const std::string& file_path, 
                                   const std::string& capture_type, 
                                   const std::string& summary,
                                   const std::string& db_path);

private:
    StorageManager& storage_;
    std::thread capture_thread_;
    std::thread continuous_thread_;
    std::atomic<bool> is_capturing_;
    std::atomic<bool> is_continuous_capturing_;
    
    void capture_loop(const std::string& file_path);
    void continuous_capture_loop(int interval_seconds);
};