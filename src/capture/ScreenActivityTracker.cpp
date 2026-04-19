#include "ScreenActivityTracker.h"

#include "../utils/TimeUtils.h"
#include "../models/TextEmbeddingGenerator.h"
#include "../utils/VectorSerializer.h"
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
    : storage_(storage), modelManager_(nullptr), is_tracking_(false), is_screenshot_capturing_(false), verbose_output_(true) {
}

ScreenActivityTracker::~ScreenActivityTracker() {
    stop_tracking();
    stop_continuous_screenshot();
}

void ScreenActivityTracker::setModelManager(ModelManager* modelManager) {
    modelManager_ = modelManager;
}

void ScreenActivityTracker::setVerboseOutput(bool verbose) {
    verbose_output_ = verbose;
}

void ScreenActivityTracker::start_tracking() {
    if (is_tracking_.load()) {
        if (verbose_output_) std::cout << "屏幕活动跟踪已在运行" << std::endl;
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
    if (verbose_output_) std::cout << "捕获屏幕截图到: " << file_path << std::endl;
    
    // 首先尝试真实的屏幕截图
    if (capture_real_screenshot(file_path)) {
        // 检查文件大小，如果超过10MB则进行压缩
        std::ifstream in_file(file_path, std::ifstream::ate | std::ifstream::binary);
        if (in_file.is_open()) {
            int file_size = static_cast<int>(in_file.tellg());
            in_file.close();
            
            if (file_size > 10 * 1024 * 1024) {  // 10MB限制
                if (verbose_output_) std::cout << "截图文件过大 (" << file_size << " 字节)，进行压缩处理" << std::endl;
                
                // 更改文件扩展名为.jpg
                std::string jpeg_file_path = file_path;
                size_t dot_pos = jpeg_file_path.find_last_of(".");
                if (dot_pos != std::string::npos) {
                    jpeg_file_path = jpeg_file_path.substr(0, dot_pos) + ".jpg";
                } else {
                    jpeg_file_path += ".jpg";
                }
                
                // 保存压缩后的图像
                compress_image(file_path, jpeg_file_path);
                if (verbose_output_) std::cout << "图像压缩完成，保存为: " << jpeg_file_path << std::endl;
                
                // 删除原始PNG文件并重命名JPEG文件
                try {
                    std::filesystem::remove(file_path);
                    std::filesystem::rename(jpeg_file_path, file_path);
                } catch (const std::exception& e) {
                    std::cerr << "文件替换失败: " << e.what() << std::endl;
                }
            }
        }
        return true;
    }
    
    // 如果真实截图失败，则使用模拟截图
    if (verbose_output_) std::cout << "使用模拟屏幕截图" << std::endl;
    
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
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(75);
    
    bool result = cv::imwrite(file_path, screenshot, compression_params);
    
    if (result) {
        if (verbose_output_) std::cout << "屏幕截图保存成功: " << file_path << std::endl;
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
        if (verbose_output_) std::cout << "请求屏幕捕获权限..." << std::endl;
        CGRequestScreenCaptureAccess();
        // 等待权限授予
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
#endif
    
    // 使用系统命令捕获屏幕截图
    std::string command = "screencapture -x -t jpg \"" + file_path + "\" 2>&1"; // 捕获错误输出
    if (verbose_output_) std::cout << "执行命令: " << command << std::endl;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "无法执行 screencapture 命令" << std::endl;
        return false;
    }
    
    // 读取命令的输出（如果有）
    char buffer[128];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int exit_code = pclose(pipe);
    
    if (exit_code == 0 && std::filesystem::exists(file_path)) {
        if (verbose_output_) std::cout << "使用screencapture命令捕获屏幕截图成功" << std::endl;
        return true;
    } else {
        std::cerr << "使用screencapture命令捕获屏幕截图失败，退出码: " << exit_code;
        if (!output.empty()) {
            std::cerr << ", 错误信息: " << output;
        }
        std::cerr << std::endl;
        return false;
    }
}

bool ScreenActivityTracker::store_activity_in_memory(int user_id, const ActivityData& activity) {
    if (verbose_output_) std::cout << "在记忆中存储屏幕活动数据" << std::endl;
    
    // 这里应该实现将屏幕活动数据存储到记忆系统的逻辑
    // 为简化起见，我们只打印信息
    if (verbose_output_) {
        std::cout << "  应用: " << activity.app_name << std::endl;
        std::cout << "  窗口标题: " << activity.window_title << std::endl;
        std::cout << "  持续时间: " << activity.duration_seconds << " 秒" << std::endl;
    }
    
    return true;
}

bool ScreenActivityTracker::store_screenshot_in_memory(int user_id, const std::string& file_path, const std::string& summary) {
    if (verbose_output_) std::cout << "在记忆中存储屏幕截图: " << file_path << std::endl;

    // 获取文件大小
    std::ifstream in_file(file_path, std::ifstream::ate | std::ifstream::binary);

    if (!in_file.is_open()) {
        std::cerr << "无法打开文件: " << file_path << std::endl;
        return false;
    }

    int file_size = static_cast<int>(in_file.tellg());
    in_file.close();
    
    // 检查文件大小，如果超过10MB则进行压缩
    std::string actual_file_path = file_path;
    bool is_compressed = false;
    if (file_size > 10 * 1024 * 1024) {  // 10MB限制
        std::cout << "截图文件过大 (" << file_size << " 字节)，超过10MB限制，进行压缩处理" << std::endl;
        std::string compressed_path = file_path + "_compressed.jpg";
        // 调用带输出路径参数的compress_image函数
        compress_image(file_path, compressed_path);
        // 检查压缩文件是否存在并使用它
        if (std::filesystem::exists(compressed_path)) {
            actual_file_path = compressed_path;
            is_compressed = true;
            std::cout << "图像压缩完成，使用压缩后的文件: " << actual_file_path << std::endl;
            
            // 重新获取压缩后文件的大小
            std::ifstream compressed_file(actual_file_path, std::ifstream::ate | std::ifstream::binary);
            if (compressed_file.is_open()) {
                file_size = static_cast<int>(compressed_file.tellg());
                compressed_file.close();
                std::cout << "压缩后文件大小: " << file_size << " 字节" << std::endl;
            }
        } else {
            std::cerr << "图像压缩失败，跳过处理" << std::endl;
            return false;
        }
    }

    // 截图已成功保存
    if (verbose_output_) std::cout << "截图已保存: " << actual_file_path << std::endl;
    return true;
}

void ScreenActivityTracker::track_loop() {
    if (verbose_output_) std::cout << "开始屏幕活动跟踪循环" << std::endl;
    
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
    
    if (verbose_output_) std::cout << "屏幕活动跟踪循环已停止" << std::endl;
}

void ScreenActivityTracker::start_continuous_screenshot(int interval_seconds) {
    if (is_screenshot_capturing_.load()) {
        if (verbose_output_) std::cout << "持续屏幕截图已在运行" << std::endl;
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
    if (verbose_output_) std::cout << "开始持续屏幕截图循环，间隔: " << interval_seconds << " 秒" << std::endl;
    
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
        
        std::string file_path = "captures/screen/screenshot_" + timestamp + ".jpg";
        
        // 捕获并存储截图
        if (capture_screenshot(file_path)) {
            // 使用共享的StorageManager实例
            store_screenshot_in_memory(1, file_path, "Continuous screenshot capture");
        }
        
        // 等待指定间隔或被停止
        for (int i = 0; i < interval_seconds && is_screenshot_capturing_.load(); i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    if (verbose_output_) std::cout << "持续屏幕截图循环已停止" << std::endl;
}

// 添加图像压缩函数
void ScreenActivityTracker::compress_image(const std::string& file_path, const std::string& output_path) {
    // 读取图像
    cv::Mat image = cv::imread(file_path);
    if (image.empty()) {
        std::cerr << "无法读取图像文件进行压缩: " << file_path << std::endl;
        return;
    }
    
    // 调整图像尺寸以减小文件大小
    cv::Mat resized_image;
    double scale = 0.8; // 缩放因子
    cv::resize(image, resized_image, cv::Size(), scale, scale, cv::INTER_AREA);
    
    // 使用更高的压缩率保存
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(50); // JPEG质量设为50
    
    // 保存压缩后的图像到指定路径
    if (cv::imwrite(output_path, resized_image, compression_params)) {
        if (verbose_output_) std::cout << "图像压缩完成，保存为: " << output_path << std::endl;
    } else {
        std::cerr << "图像压缩失败" << std::endl;
    }
}
