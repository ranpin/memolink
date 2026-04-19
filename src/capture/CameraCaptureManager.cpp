#include "CameraCaptureManager.h"
#include "../utils/TimeUtils.h"
#include "../models/TextEmbeddingGenerator.h"
#include "../utils/VectorSerializer.h"
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

// 添加互斥锁以确保线程安全
static std::mutex g_camera_mutex;

CameraCaptureManager::CameraCaptureManager(StorageManager& storage) 
    : storage_(storage), is_capturing_(false), is_continuous_capturing_(false) {
}

CameraCaptureManager::~CameraCaptureManager() {
    stop_capture();
    stop_continuous_capture();
}

bool CameraCaptureManager::capture_image(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(g_camera_mutex);
    
    // 确保目录存在
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());
    
    std::cout << "[CameraCaptureManager] 尝试打开摄像头..." << std::endl;
    
    // 使用OpenCV打开摄像头（默认摄像头ID为0）
    cv::VideoCapture camera(0);
    
    if (!camera.isOpened()) {
        std::cerr << "[CameraCaptureManager] 无法打开摄像头，尝试备用方案" << std::endl;
        
        // 备用方案：使用屏幕截图
        std::string command = "screencapture -x -o " + file_path + " 2>/dev/null";
        int result = system(command.c_str());
        
        if (result == 0 && std::filesystem::exists(file_path)) {
            std::cout << "[CameraCaptureManager] 使用屏幕截图作为备用，保存成功: " << file_path << std::endl;
            return true;
        } else {
            std::cerr << "[CameraCaptureManager] 保存图像失败: " << file_path << std::endl;
            return false;
        }
    }
    
    // 设置摄像头分辨率
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    
    // 等待摄像头稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    cv::Mat frame;
    bool success = camera.read(frame);
    
    if (!success || frame.empty()) {
        std::cerr << "[CameraCaptureManager] 读取摄像头帧失败" << std::endl;
        camera.release();
        return false;
    }
    
    // 保存为JPEG格式
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(90);
    
    success = cv::imwrite(file_path, frame, compression_params);
    camera.release();
    
    if (success) {
        std::cout << "[CameraCaptureManager] ✓ 成功从摄像头捕获并保存图像: " << file_path 
                  << " (" << frame.cols << "x" << frame.rows << ")" << std::endl;
        return true;
    } else {
        std::cerr << "[CameraCaptureManager] 保存摄像头图像失败: " << file_path << std::endl;
        return false;
    }
}

bool CameraCaptureManager::start_video_capture(const std::string& file_path) {
    // 确保目录存在
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());
    
    // 启动视频捕获线程
    is_capturing_ = true;
    capture_thread_ = std::thread(&CameraCaptureManager::capture_loop, this, file_path);
    std::cout << "开始视频捕获到: " << file_path << std::endl;
    return true;
}

void CameraCaptureManager::stop_capture() {
    std::cout << "Stopping video capture" << std::endl;
    
    if (is_capturing_.load()) {
        is_capturing_.store(false);
        
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
    }
}

void CameraCaptureManager::capture_loop(const std::string& file_path) {
    std::cout << "[CameraCaptureManager] 开始摄像头捕获循环..." << std::endl;
    
    // 打开摄像头
    cv::VideoCapture camera(0);
    
    if (!camera.isOpened()) {
        std::cerr << "[CameraCaptureManager] 无法打开摄像头，视频捕获循环终止" << std::endl;
        is_capturing_.store(false);
        return;
    }
    
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    
    int frame_count = 0;
    while (is_capturing_) {
        // 生成带时间戳的文件名
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << "captures/camera/image_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
           << "_" << std::setfill('0') << std::setw(3) << ms.count() << ".jpg";
        
        std::string image_path = ss.str();
        
        // 确保目录存在
        std::filesystem::path path(image_path);
        std::filesystem::create_directories(path.parent_path());
        
        cv::Mat frame;
        if (camera.read(frame) && !frame.empty()) {
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(90);
            
            if (cv::imwrite(image_path, frame, compression_params)) {
                std::cout << "[CameraCaptureManager] 成功捕获帧 #" << frame_count 
                          << ": " << image_path << std::endl;
            } else {
                std::cerr << "[CameraCaptureManager] 保存帧失败: " << image_path << std::endl;
            }
        } else {
            std::cerr << "[CameraCaptureManager] 读取摄像头帧失败" << std::endl;
        }
        
        frame_count++;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒捕获一帧
    }
    
    camera.release();
    std::cout << "[CameraCaptureManager] 摄像头捕获循环结束，共捕获 " << frame_count << " 帧" << std::endl;
}

void CameraCaptureManager::start_continuous_capture(int interval_seconds) {
    if (is_continuous_capturing_.load()) {
        std::cout << "持续摄像头捕获已在运行" << std::endl;
        return;
    }
    
    is_continuous_capturing_.store(true);
    continuous_thread_ = std::thread(&CameraCaptureManager::continuous_capture_loop, this, interval_seconds);
}

void CameraCaptureManager::stop_continuous_capture() {
    if (is_continuous_capturing_.load()) {
        is_continuous_capturing_.store(false);
        
        if (continuous_thread_.joinable()) {
            continuous_thread_.join();
        }
    }
}

void CameraCaptureManager::continuous_capture_loop(int interval_seconds) {
    std::cout << "开始持续摄像头捕获循环，间隔: " << interval_seconds << " 秒" << std::endl;
    
    while (is_continuous_capturing_.load()) {
        // 生成带时间戳的文件名
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
        std::string timestamp = ss.str();
        
        std::string file_path = "captures/camera/image_" + timestamp + ".jpg";
        
        // 捕获并存储图像
        if (capture_image(file_path)) {
            store_capture_in_memory(1, file_path, "image", "Continuous camera capture");
        }
        
        // 等待指定间隔或被停止
        for (int i = 0; i < interval_seconds && is_continuous_capturing_.load(); i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "持续摄像头捕获循环已停止" << std::endl;
}

bool CameraCaptureManager::store_capture_in_memory(int user_id, 
                                const std::string& file_path,
                                const std::string& capture_type,
                                const std::string& summary) {
    // 这里不再写入ResourceMemory，仅打印日志，实际的多模态存储由MultimodalDataManager完成
    std::cout << "[CameraCaptureManager] store_capture_in_memory stub - user "
              << user_id << ", type " << capture_type
              << ", file " << file_path << std::endl;
    (void)summary;
    return true;
}