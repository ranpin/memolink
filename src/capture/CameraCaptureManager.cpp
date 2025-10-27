#include "CameraCaptureManager.h"
#include "../database/managers/ResourceMemoryManager.h"
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
    // 确保目录存在
    std::filesystem::path path(file_path);
    std::filesystem::create_directories(path.parent_path());
    
    // 使用系统命令捕获图像（模拟）
    std::string command = "screencapture -x -o " + file_path + " 2>/dev/null";
    int result = system(command.c_str());
    
    if (result == 0 && std::filesystem::exists(file_path)) {
        std::cout << "成功捕获并保存图像: " << file_path << std::endl;
        return true;
    } else {
        std::cerr << "保存图像失败: " << file_path << std::endl;
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
        
        // 捕获图像
        std::string command = "screencapture -x -o " + image_path + " 2>/dev/null";
        int result = system(command.c_str());
        
        if (result == 0 && std::filesystem::exists(image_path)) {
            std::cout << "成功捕获并保存图像: " << image_path << std::endl;
        } else {
            std::cerr << "图像捕获失败: " << image_path << std::endl;
        }
        
        frame_count++;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒捕获一帧
    }
}

int CameraCaptureManager::saveCaptureToResourceMemory(
    int user_id, 
    const std::string& file_path, 
    const std::string& capture_type, 
    const std::string& summary,
    const std::string& db_path) {
    
    // 获取文件大小
    std::ifstream in(file_path, std::ifstream::ate | std::ifstream::binary);
    int file_size = in.tellg();
    in.close();
    
    // 使用已有的StorageManager存储捕获数据，而不是创建新的实例
    ResourceMemoryManager manager(storage_);
    
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = 1; // 默认agent_id
    item.description = "Captured camera content: " + file_path;  // 将title和content合并为description
    item.resource_type = capture_type;
    item.path = file_path;  // 使用文件路径作为path
    item.metadata = "{}";
    item.size = 0;  // 默认大小为0
    
    bool result = manager.add_resource_memory(item);
    return result ? 1 : -1; // 简化处理，实际应该返回插入记录的ID
}

bool CameraCaptureManager::store_capture_in_memory(int user_id, const std::string& file_path, 
                                                  const std::string& capture_type, 
                                                  const std::string& summary) {
    // 创建ResourceMemoryManager实例
    ResourceMemoryManager resourceMemoryManager(storage_);
    
    // 使用TextEmbeddingGenerator生成图像描述
    std::string description = summary; // 默认使用摘要作为描述
    try {
        TextEmbeddingGenerator textEmbeddingGenerator(&storage_);
        description = textEmbeddingGenerator.generateImageDescription(file_path);
        std::cout << "[摄像头捕获] 图像描述生成成功: " << description << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[摄像头捕获] 图像描述生成失败: " << e.what() << std::endl;
        std::cout << "[摄像头捕获] 使用默认摘要作为描述" << std::endl;
    }
    
    // 使用ResourceMemoryManager存储捕获的图像数据
    ResourceMemoryManager::ResourceMemoryItem item;
    item.user_id = user_id;
    item.agent_id = 0; // 默认agent_id为0
    item.session_id = 0; // 默认session_id为0
    item.description = description; // 使用生成的描述
    item.resource_type = capture_type;
    item.path = file_path;
    item.metadata = "{}"; // 空的元数据
    item.embedding = ""; // 默认空的embedding
    item.size = 0; // 默认大小为0
    item.created_at = TimeUtils::getCurrentTimestamp(); // 获取当前时间戳
    
    // 为图像描述生成embedding并存储到资源记忆中
    try {
        TextEmbeddingGenerator textEmbeddingGenerator(&storage_);
        std::vector<float> descriptionEmbedding = textEmbeddingGenerator.generateEmbedding(description);
        if (!descriptionEmbedding.empty()) {
            // 将embedding序列化并存储到item中
            item.embedding = VectorSerializer::serialize(descriptionEmbedding);
            std::cout << "[摄像头捕获] 图像描述embedding生成成功，维度: " << descriptionEmbedding.size() << std::endl;
        } else {
            std::cout << "[摄像头捕获] 图像描述embedding生成失败，返回空向量" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[摄像头捕获] 图像描述embedding生成异常: " << e.what() << std::endl;
    }
    
    std::cout << "[摄像头捕获] 准备存储的embedding大小: " << item.embedding.size() << " 字节" << std::endl;
    
    return resourceMemoryManager.add_resource_memory(item);
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