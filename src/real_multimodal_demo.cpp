#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <ctime>
#include "capture/CameraCaptureManager.h"
#include "capture/AudioCaptureManager.h"
#include "capture/ScreenActivityTracker.h"
#include "capture/MultimodalDataManager.h"
#include "models/ModelManager.h"
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/initialization/ConfigManager/ConfigManager.h"

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
    
    // 初始化多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 初始化各个捕获管理器
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

int main() {
    std::cout << "=== MIRIX 真实多模态数据处理演示 ===" << std::endl;
    
    // 确保数据库目录存在
    std::filesystem::create_directories("../database");
    
    // 初始化数据库
    if (!InitDB::initializeDatabase("../database/mirix.db")) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return 1;
    }
    
    std::cout << "数据库初始化成功: mirix.db" << std::endl;
    
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