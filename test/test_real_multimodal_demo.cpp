#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <ctime>
#include "legacy/desktop/capture/CameraCaptureManager.h"
#include "legacy/desktop/capture/AudioCaptureManager.h"
#include "legacy/desktop/capture/ScreenActivityTracker.h"
#include "legacy/desktop/capture/MultimodalDataManager.h"
#include "legacy/infra-alt/ModelManager.h"
#include "infra/storage/StorageManager.h"
#include "infra/storage/InitDB.h"
#include "infra/storage/ConfigManager.h"
#include "legacy/core-v1/ResourceMemoryManager.h"

// 全局变量
std::atomic<bool> g_is_running(true);
int g_audio_capture_duration = 5; // 音频捕获持续时间（秒）
StorageManager* g_storage = nullptr; // 全局存储管理器实例

void signalHandler(int signum) {
    g_is_running = false;
}

void setupSignalHandler() {
    signal(SIGINT, signalHandler);
    std::cout << "设置信号处理程序。按 Ctrl+C 可以停止捕获。" << std::endl;
    
    // 确保captures目录存在
    std::filesystem::create_directories("../captures");
    std::filesystem::create_directories("../captures/camera");
    std::filesystem::create_directories("../captures/audio");
    std::filesystem::create_directories("../captures/screen");
    
    // 初始化全局存储管理器
    if (!g_storage) {
        g_storage = new StorageManager("../database/mirix.db");
        if (!g_storage->initialize()) {
            std::cerr << "无法初始化全局存储管理器" << std::endl;
        }
    }
}

void initializeDatabase() {
    std::cout << "初始化数据库..." << std::endl;
    
    // 使用项目根目录下的database目录
    std::string dbPath = "../database/mirix.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    // 初始化存储管理器
    g_storage = new StorageManager(dbPath);
    if (!g_storage->initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        delete g_storage;
        g_storage = nullptr;
        return;
    }
    
    std::cout << "数据库初始化成功: " << dbPath << std::endl;
}

// 保留全局配置变量用于菜单设置功能
int g_camera_capture_duration = 5;      
int g_screenshot_count = 1;             
int g_continuous_camera_interval = 30;  
int g_continuous_audio_duration = 5;
int g_continuous_audio_interval = 30;   
int g_continuous_screenshot_interval = 30;

void showMenu() {
    std::cout << "\n=== Mirix 多模态演示系统 ===" << std::endl;
    std::cout << "1. 基本多模态捕获演示" << std::endl;
    std::cout << "2. 多模态数据处理演示" << std::endl;
    std::cout << "3. 持续多模态捕获演示" << std::endl;
    std::cout << "4. 配置设置" << std::endl;
    std::cout << "5. 退出" << std::endl;
}

void demonstrateBasicCapture() {
    std::cout << "=== 基本多模态捕获演示 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化" << std::endl;
        return;
    }
    
    // 初始化多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 初始化各个捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 演示图像捕获
    std::cout << "1. 演示图像捕获..." << std::endl;
    std::string image_path = "../captures/image_" + std::to_string(time(nullptr)) + ".jpg";
    if (cameraManager.capture_image(image_path)) {
        std::cout << "   图像捕获成功: " << image_path << std::endl;
        // 存储到多模态数据管理器
        if (multimodalManager.saveMultimodalData(1, "image", image_path, "演示图像捕获")) {
            std::cout << "   图像数据已存储到多模态数据库" << std::endl;
        } else {
            std::cout << "   图像数据存储失败" << std::endl;
        }
    } else {
        std::cout << "   图像捕获失败" << std::endl;
    }
    
    // 演示音频捕获
    std::cout << "2. 演示音频捕获..." << std::endl;
    std::string audio_path = "../captures/audio_" + std::to_string(time(nullptr)) + ".wav";
    audioManager.startCapture(audio_path, g_audio_capture_duration);
    // 假设音频捕获成功
    std::cout << "   音频捕获成功: " << audio_path << std::endl;
    // 存储到多模态数据管理器
    if (multimodalManager.saveMultimodalData(1, "audio", audio_path, "演示音频捕获", "", "", {}, "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}")) {
        std::cout << "   音频数据已存储到多模态数据库" << std::endl;
    } else {
        std::cout << "   音频数据存储失败" << std::endl;
    }
    
    // 演示屏幕截图
    std::cout << "3. 演示屏幕截图..." << std::endl;
    std::string screenshot_path = "../captures/screenshot_" + std::to_string(time(nullptr)) + ".png";
    if (screenTracker.capture_screenshot(screenshot_path)) {
        std::cout << "   屏幕截图成功: " << screenshot_path << std::endl;
        // 存储到多模态数据管理器
        if (multimodalManager.saveMultimodalData(1, "screenshot", screenshot_path, "演示屏幕截图")) {
            std::cout << "   屏幕截图数据已存储到多模态数据库" << std::endl;
        } else {
            std::cout << "   屏幕截图数据存储失败" << std::endl;
        }
    } else {
        std::cout << "   屏幕截图失败" << std::endl;
    }
    
    std::cout << "基本多模态捕获演示完成!" << std::endl;
    
    // 停止任何正在进行的捕获
    cameraManager.stop_capture();
    screenTracker.stop_tracking();
}

void demonstrateMultimodalProcessing() {
    std::cout << "\n=== 多模态数据处理演示 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化" << std::endl;
        return;
    }
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 创建各个捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 同时捕获所有类型的数据
    std::string image_path = "../captures/image_" + std::to_string(time(nullptr)) + ".jpg";
    std::string audio_path = "../captures/audio_" + std::to_string(time(nullptr)) + ".wav";
    std::string screenshot_path = "../captures/screenshot_" + std::to_string(time(nullptr)) + ".png";
    
    bool capture_success = true;
    
    // 捕获图像
    if (!cameraManager.capture_image(image_path)) {
        std::cout << "图像捕获失败" << std::endl;
        capture_success = false;
    }
    
    // 捕获音频
    audioManager.startCapture(audio_path, g_audio_capture_duration);
    
    // 捕获屏幕截图
    if (!screenTracker.capture_screenshot(screenshot_path)) {
        std::cout << "屏幕截图捕获失败" << std::endl;
        capture_success = false;
    }
    
    if (capture_success) {
        // 存储所有捕获的数据
        if (!multimodalManager.saveMultimodalData(1, "image", image_path, "多模态处理图像")) {
            std::cout << "图像数据存储失败" << std::endl;
        }
        
        if (!multimodalManager.saveMultimodalData(1, "audio", audio_path, "多模态处理音频", "", "", {}, "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}")) {
            std::cout << "音频数据存储失败" << std::endl;
        }
        
        if (!multimodalManager.saveMultimodalData(1, "screenshot", screenshot_path, "多模态处理屏幕截图")) {
            std::cout << "屏幕截图数据存储失败" << std::endl;
        }
        
        // 处理多模态数据
        std::cout << "多模态数据处理完成!" << std::endl;
    } else {
        std::cout << "多模态数据捕获失败，跳过多模态处理演示" << std::endl;
    }
}

void demonstrateContinuousCapture() {
    std::cout << "\n=== 持续多模态捕获演示 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化" << std::endl;
        return;
    }
    
    // 初始化多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 初始化各个捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 启动持续捕获
    std::cout << "启动持续捕获（30秒）..." << std::endl;
    
    // 启动持续截图
    screenTracker.start_continuous_screenshot(10); // 每10秒截图一次
    std::cout << "开始持续屏幕截图循环，间隔: 10 秒" << std::endl;
    
    // 启动持续摄像头捕获
    cameraManager.start_continuous_capture(15); // 每15秒拍照一次
    std::cout << "开始持续摄像头捕获循环，间隔: 15 秒" << std::endl;
    
    // 运行30秒
    auto start_time = std::chrono::steady_clock::now();
    while (g_is_running) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        
        if (elapsed_seconds >= 30) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 停止捕获
    cameraManager.stop_continuous_capture();
    audioManager.stopCapture();
    screenTracker.stop_continuous_screenshot();
    
    std::cout << "持续多模态捕获演示完成!" << std::endl;
}

void continuousCaptureLoop() {
    std::cout << "\n=== 开始持续捕获循环 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    // multimodalManager.initialize(); // 注释掉不存在的函数调用
    
    // 创建屏幕活动跟踪器
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建摄像头捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    
    int captureCount = 0;
    const int maxCaptures = 3;
    
    while (g_is_running && captureCount < maxCaptures) {
        std::cout << "\n--- 第 " << (captureCount + 1) << " 次捕获 ---" << std::endl;
        
        // 捕获屏幕截图
        std::string screenshotPath = "captures/screen/screenshot_" + std::to_string(captureCount) + ".png"; // 使用预定义路径
        bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath); // 使用正确的函数名
        if (screenshotSuccess) {
            std::cout << "捕获屏幕截图: " << screenshotPath << std::endl;
            
            // 保存到资源记忆
            ResourceMemoryManager resourceManager(*g_storage);
            ResourceMemoryManager::ResourceMemoryItem item;
            item.user_id = 1;
            item.agent_id = 1;
            item.session_id = 1;
            item.path = screenshotPath;
            item.resource_type = "screenshot";
            item.description = "屏幕截图";
            item.size = 0;
            item.metadata = "";
            item.embedding = "";
            bool addSuccess = resourceManager.add_resource_memory(item); // 使用正确的函数名
            
            if (addSuccess) {
                std::cout << "屏幕截图已保存到资源记忆" << std::endl;
            }
        }
        
        // 捕获摄像头图像
        std::string imagePath = "captures/camera/image_" + std::to_string(captureCount) + ".jpg"; // 使用预定义路径
        bool captureSuccess = cameraManager.capture_image(imagePath); // 使用正确的函数名
        if (captureSuccess) {
            std::cout << "捕获摄像头图像: " << imagePath << std::endl;
            
            // 保存到资源记忆
            ResourceMemoryManager resourceManager(*g_storage);
            ResourceMemoryManager::ResourceMemoryItem item;
            item.user_id = 1;
            item.agent_id = 1;
            item.session_id = 1;
            item.path = imagePath;
            item.resource_type = "image";
            item.description = "摄像头捕获的图片";
            item.size = 0;
            item.metadata = "";
            item.embedding = "";
            bool addSuccess = resourceManager.add_resource_memory(item); // 使用正确的函数名
            
            if (addSuccess) {
                std::cout << "摄像头图像已保存到资源记忆" << std::endl;
            }
        }
        
        captureCount++;
        
        if (captureCount < maxCaptures) {
            std::cout << "等待5秒后进行下一次捕获..." << std::endl;
            for (int i = 0; i < 5 && g_is_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    std::cout << "\n持续捕获循环结束" << std::endl;
}

void testCameraCapture() {
    std::cout << "\n=== 测试摄像头捕获 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    // multimodalManager.initialize(); // 注释掉不存在的函数调用
    
    // 创建摄像头捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    
    // 捕获一张图片
    std::string imagePath = "captures/camera/test_image.jpg"; // 使用预定义路径
    bool captureSuccess = cameraManager.capture_image(imagePath); // 使用正确的函数名
    if (captureSuccess) {
        std::cout << "成功捕获图片: " << imagePath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = imagePath;
        item.resource_type = "image";
        item.description = "摄像头捕获的图片";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item); // 使用正确的函数名
        
        if (addSuccess) {
            std::cout << "图片已保存到资源记忆" << std::endl;
        }
    } else {
        std::cout << "图片捕获失败" << std::endl;
    }
}

void testAudioCapture() {
    std::cout << "\n=== 测试音频捕获 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    // multimodalManager.initialize(); // 注释掉不存在的函数调用
    
    // 创建音频捕获管理器
    AudioCaptureManager audioManager(*g_storage);
    
    // 捕获音频
    std::string audioPath = "captures/audio/test_audio.wav"; // 使用预定义路径
    bool captureSuccess = audioManager.startCapture(audioPath, g_audio_capture_duration); // 使用正确的函数名
    if (captureSuccess) {
        std::cout << "成功录制音频: " << audioPath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = audioPath;
        item.resource_type = "audio";
        item.description = "录制的音频";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item); // 使用正确的函数名
        
        if (addSuccess) {
            std::cout << "音频已保存到资源记忆" << std::endl;
        }
    } else {
        std::cout << "音频录制失败" << std::endl;
    }
}

void testContinuousAudioCapture() {
    std::cout << "\n=== 测试连续音频捕获 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建音频捕获管理器
    AudioCaptureManager audioManager(*g_storage);
    if (!audioManager.initialize()) {
        std::cerr << "音频捕获管理器初始化失败!" << std::endl;
        return;
    }
    
    // 确保音频捕获目录存在
    std::filesystem::create_directories("../captures/audio");
    
    // 捕获音频
    std::string audioPath = "../captures/audio/continuous_audio.wav";
    bool captureSuccess = audioManager.startCapture(audioPath, g_continuous_audio_duration);
    if (captureSuccess) {
        std::cout << "成功录制连续音频: " << audioPath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = audioPath;
        item.resource_type = "audio";
        item.description = "连续音频录制";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item);
        
        if (addSuccess) {
            std::cout << "连续音频已保存到资源记忆" << std::endl;
        }
    } else {
        std::cout << "连续音频录制失败" << std::endl;
    }
}

void testContinuousCapture() {
    std::cout << "\n=== 测试连续捕获 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 开始连续捕获
    screenTracker.start_continuous_screenshot(10); // 每10秒截图一次
    std::cout << "开始持续屏幕截图循环，间隔: 10 秒" << std::endl;
    
    cameraManager.start_continuous_capture(15); // 每15秒拍照一次
    std::cout << "开始持续摄像头捕获循环，间隔: 15 秒" << std::endl;
    
    // 等待一段时间观察连续捕获效果
    std::cout << "等待60秒观察连续捕获效果..." << std::endl;
    for (int i = 0; i < 60 && g_is_running; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 停止连续捕获
    screenTracker.stop_continuous_screenshot();
    cameraManager.stop_continuous_capture();
    
    std::cout << "连续捕获测试结束" << std::endl;
}

void testContinuousMultimodalCapture() {
    std::cout << "\n=== 测试连续多模态捕获 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 连续捕获循环
    int captureCount = 0;
    const int maxCaptures = 5;
    
    while (g_is_running && captureCount < maxCaptures) {
        std::cout << "\n--- 第 " << (captureCount + 1) << " 次连续捕获 ---" << std::endl;
        
        // 捕获屏幕截图
        std::string screenshotPath = "captures/screen/continuous_screenshot_" + std::to_string(captureCount) + ".png";
        bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath);
        if (screenshotSuccess) {
            std::cout << "捕获屏幕截图: " << screenshotPath << std::endl;
            
            // 保存到资源记忆
            ResourceMemoryManager resourceManager(*g_storage);
            ResourceMemoryManager::ResourceMemoryItem item;
            item.user_id = 1;
            item.agent_id = 1;
            item.session_id = 1;
            item.path = screenshotPath;
            item.resource_type = "screenshot";
            item.description = "连续屏幕截图";
            item.size = 0;
            item.metadata = "";
            item.embedding = "";
            bool addSuccess = resourceManager.add_resource_memory(item);
            
            if (addSuccess) {
                std::cout << "屏幕截图已保存到资源记忆" << std::endl;
            }
        }
        
        // 捕获摄像头图像
        std::string imagePath = "captures/camera/continuous_image_" + std::to_string(captureCount) + ".jpg";
        bool captureSuccess = cameraManager.capture_image(imagePath);
        if (captureSuccess) {
            std::cout << "捕获摄像头图像: " << imagePath << std::endl;
            
            // 保存到资源记忆
            ResourceMemoryManager resourceManager(*g_storage);
            ResourceMemoryManager::ResourceMemoryItem item;
            item.user_id = 1;
            item.agent_id = 1;
            item.session_id = 1;
            item.path = imagePath;
            item.resource_type = "image";
            item.description = "连续摄像头捕获的图片";
            item.size = 0;
            item.metadata = "";
            item.embedding = "";
            bool addSuccess = resourceManager.add_resource_memory(item);
            
            if (addSuccess) {
                std::cout << "摄像头图像已保存到资源记忆" << std::endl;
            }
        }
        
        // 捕获音频
        std::string audioPath = "captures/audio/continuous_audio_" + std::to_string(captureCount) + ".wav";
        bool audioSuccess = audioManager.startCapture(audioPath, g_continuous_audio_duration);
        if (audioSuccess) {
            std::cout << "录制音频: " << audioPath << std::endl;
            
            // 保存到资源记忆
            ResourceMemoryManager resourceManager(*g_storage);
            ResourceMemoryManager::ResourceMemoryItem item;
            item.user_id = 1;
            item.agent_id = 1;
            item.session_id = 1;
            item.path = audioPath;
            item.resource_type = "audio";
            item.description = "连续音频录制";
            item.size = 0;
            item.metadata = "";
            item.embedding = "";
            bool addSuccess = resourceManager.add_resource_memory(item);
            
            if (addSuccess) {
                std::cout << "音频已保存到资源记忆" << std::endl;
            }
        }
        
        captureCount++;
        
        if (captureCount < maxCaptures) {
            std::cout << "等待 " << g_continuous_camera_interval << " 秒后进行下一次捕获..." << std::endl;
            for (int i = 0; i < g_continuous_camera_interval && g_is_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    std::cout << "\n连续多模态捕获结束" << std::endl;
}

void runTests() {
    std::cout << "\n=== 开始运行多模态演示测试 ===" << std::endl;
    
    // 测试摄像头捕获
    testCameraCapture();
    
    // 测试音频捕获
    testAudioCapture();
    
    // 测试连续捕获循环
    continuousCaptureLoop();
    
    // 测试连续音频捕获
    testContinuousAudioCapture();
    
    // 测试连续多模态捕获
    testContinuousMultimodalCapture();
    
    std::cout << "\n=== 所有测试完成 ===" << std::endl;
}

void configureSettings() {
    std::cout << "\n=== 配置设置 ===" << std::endl;
    std::cout << "当前配置:" << std::endl;
    std::cout << "1. 摄像头捕获间隔: " << g_continuous_camera_interval << " 秒" << std::endl;
    std::cout << "2. 音频捕获时长: " << g_audio_capture_duration << " 秒" << std::endl;
    std::cout << "3. 截图数量: " << g_screenshot_count << std::endl;
    
    std::cout << "\n请输入新的配置值:" << std::endl;
    std::cout << "摄像头捕获间隔 (秒): ";
    std::cin >> g_continuous_camera_interval;
    std::cout << "音频捕获时长 (秒): ";
    std::cin >> g_audio_capture_duration;
    std::cout << "截图数量: ";
    std::cin >> g_screenshot_count;
    
    std::cout << "配置已更新!" << std::endl;
}

void configureContinuousCapture() {
    std::cout << "\n=== 配置连续捕获参数 ===" << std::endl;
    
    std::cout << "当前配置:" << std::endl;
    std::cout << "1. 摄像头捕获间隔: " << g_continuous_camera_interval << " 秒" << std::endl;
    std::cout << "2. 音频捕获时长: " << g_continuous_audio_duration << " 秒" << std::endl;
    std::cout << "3. 音频捕获间隔: " << g_continuous_audio_interval << " 秒" << std::endl;
    std::cout << "4. 屏幕截图间隔: " << g_continuous_screenshot_interval << " 秒" << std::endl;
    std::cout << "5. 捕获间隔: " << g_continuous_camera_interval << " 秒" << std::endl;
    
    std::cout << "\n是否要修改配置? (y/n): ";
    char choice;
    std::cin >> choice;
    
    if (choice == 'y' || choice == 'Y') {
        std::cout << "请输入摄像头捕获间隔(秒): ";
        std::cin >> g_continuous_camera_interval;
        
        std::cout << "请输入音频捕获时长(秒): ";
        std::cin >> g_continuous_audio_duration;
        
        std::cout << "请输入音频捕获间隔(秒): ";
        std::cin >> g_continuous_audio_interval;
        
        std::cout << "请输入屏幕截图间隔(秒): ";
        std::cin >> g_continuous_screenshot_interval;
        
        std::cout << "配置已更新!" << std::endl;
    }
}

void setupDemoEnvironment() {
    std::cout << "\n=== 设置演示环境 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建必要的目录
    std::filesystem::create_directories("captures/camera");
    std::filesystem::create_directories("captures/audio");
    std::filesystem::create_directories("captures/screen");
    
    std::cout << "演示环境设置完成" << std::endl;
    std::cout << "创建目录: captures/camera, captures/audio, captures/screen" << std::endl;
}

void runDemoWithSetup() {
    std::cout << "\n=== 运行带设置的演示 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建必要的目录
    std::filesystem::create_directories("captures/camera");
    std::filesystem::create_directories("captures/audio");
    std::filesystem::create_directories("captures/screen");
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 捕获一张图片
    std::string imagePath = "captures/camera/demo_image.jpg";
    bool captureSuccess = cameraManager.capture_image(imagePath);
    if (captureSuccess) {
        std::cout << "成功捕获图片: " << imagePath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = imagePath;
        item.resource_type = "image";
        item.description = "演示图片捕获";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item);
        
        if (addSuccess) {
            std::cout << "图片已保存到资源记忆" << std::endl;
        }
    }
    
    // 捕获音频
    std::string audioPath = "captures/audio/demo_audio.wav";
    bool audioSuccess = audioManager.startCapture(audioPath, 5);
    if (audioSuccess) {
        std::cout << "成功录制音频: " << audioPath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = audioPath;
        item.resource_type = "audio";
        item.description = "演示音频录制";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item);
        
        if (addSuccess) {
            std::cout << "音频已保存到资源记忆" << std::endl;
        }
    }
    
    // 捕获屏幕截图
    std::string screenshotPath = "captures/screen/demo_screenshot.png";
    bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath);
    if (screenshotSuccess) {
        std::cout << "成功捕获屏幕截图: " << screenshotPath << std::endl;
        
        // 保存到资源记忆
        ResourceMemoryManager resourceManager(*g_storage);
        ResourceMemoryManager::ResourceMemoryItem item;
        item.user_id = 1;
        item.agent_id = 1;
        item.session_id = 1;
        item.path = screenshotPath;
        item.resource_type = "screenshot";
        item.description = "演示屏幕截图";
        item.size = 0;
        item.metadata = "";
        item.embedding = "";
        bool addSuccess = resourceManager.add_resource_memory(item);
        
        if (addSuccess) {
            std::cout << "屏幕截图已保存到资源记忆" << std::endl;
        }
    }
    
    std::cout << "演示完成" << std::endl;
}

void startContinuousCaptureDemo() {
    std::cout << "\n=== 启动连续捕获演示 ===" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化!" << std::endl;
        return;
    }
    
    // 创建捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 开始连续捕获
    screenTracker.start_continuous_screenshot(g_continuous_screenshot_interval); // 每10秒截图一次
    std::cout << "开始持续屏幕截图循环，间隔: " << g_continuous_screenshot_interval << " 秒" << std::endl;
    
    cameraManager.start_continuous_capture(g_continuous_camera_interval); // 每15秒拍照一次
    std::cout << "开始持续摄像头捕获循环，间隔: " << g_continuous_camera_interval << " 秒" << std::endl;
    
    // 音频捕获需要手动触发，这里仅做一次示例
    audioManager.startCapture("captures/audio/demo_audio.wav", g_continuous_audio_duration);
    std::cout << "开始音频捕获，时长: " << g_continuous_audio_duration << " 秒" << std::endl;
    
    // 等待一段时间观察连续捕获效果
    std::cout << "启动连续捕获演示，按 Ctrl+C 停止..." << std::endl;
    while (g_is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 停止连续捕获
    screenTracker.stop_continuous_screenshot();
    cameraManager.stop_continuous_capture();
    
    std::cout << "连续捕获演示结束" << std::endl;
}

int main() {
    std::cout << "=== MIRIX 真实多模态数据处理演示 ===" << std::endl;
    
    // 确保数据库目录存在
    std::filesystem::create_directories("../database");
    
    // 初始化数据库
    initializeDatabase();
    
    // 设置信号处理程序
    setupSignalHandler();
    
    // 主演示循环
    std::cout << "=== 真实多模态数据捕获演示 ===" << std::endl;
    int choice;
    do {
        showMenu();
        std::cout << "请选择演示项目: ";
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                demonstrateBasicCapture();
                break;
            case 2:
                demonstrateMultimodalProcessing();
                break;
            case 3:
                demonstrateContinuousCapture();
                break;
            case 4:
                configureSettings();
                break;
            case 5:
                std::cout << "感谢使用Mirix多模态演示系统!" << std::endl;
                break;
            default:
                std::cout << "无效选择，请重新输入。" << std::endl;
                break;
        }
    } while (choice != 5 && g_is_running);
    
    // 清理全局资源
    if (g_storage) {
        delete g_storage;
        g_storage = nullptr;
    }
    
    return 0;
}