#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <ctime>
#include <vector>
#include <limits>
#include <opencv2/opencv.hpp>
#include "capture/CameraCaptureManager.h"
#include "capture/AudioCaptureManager.h"
#include "capture/ScreenActivityTracker.h"
#include "capture/MultimodalDataManager.h"
#include "models/ModelManager.h"
#include "database/managers/MemoryProvenanceManager.h"
#include "database/core/ORMWrapper.h"
#include "database/core/StorageManager.h"
#include "database/initialization/ConfigManager.h"
#include "database/managers/MemoryOrchestratorImpl.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/models/MemoryModels.h"
#include "utils/TimeUtils.h"
#include "utils/LogFormatter.h"
#include "agents/ChatAgent.h"
#include "memory/generation/MemoryGenerationManager.h"

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
    
    // 创建数据库目录
    std::filesystem::create_directories("../database");
    
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

// 将大图压缩为用于多模态模型的JPEG小图
std::string create_resized_jpeg_for_multimodal(const std::string& input_path,
                                               const std::string& prefix,
                                               int max_size = 1024,
                                               int quality = 75) {
    try {
        cv::Mat image = cv::imread(input_path);
        if (image.empty()) {
            std::cerr << "[多模态演示] 无法读取图像文件进行压缩: " << input_path << std::endl;
            return input_path;
        }

        cv::Mat resized = image;
        int width = image.cols;
        int height = image.rows;
        double scale = 1.0;
        if (width > max_size || height > max_size) {
            double scale_w = static_cast<double>(max_size) / static_cast<double>(width);
            double scale_h = static_cast<double>(max_size) / static_cast<double>(height);
            scale = std::min(scale_w, scale_h);
            cv::resize(image, resized, cv::Size(), scale, scale, cv::INTER_AREA);
        }

        std::filesystem::path in_path(input_path);
        std::filesystem::path dir = in_path.parent_path();
        if (dir.empty()) {
            dir = ".";
        }
        std::filesystem::create_directories(dir);

        std::string base_name = prefix + "_" + std::to_string(std::time(nullptr)) + ".jpg";
        std::filesystem::path out_path = dir / base_name;

        std::vector<int> params;
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(quality);

        if (!cv::imwrite(out_path.string(), resized, params)) {
            std::cerr << "[多模态演示] 写入压缩后JPEG失败: " << out_path << std::endl;
            return input_path;
        }

        std::error_code ec;
        auto file_size = std::filesystem::file_size(out_path, ec);
        if (!ec) {
            std::cout << "[多模态演示] 生成JPEG小图: " << out_path
                      << "，大小: " << file_size << " 字节" << std::endl;
        }

        return out_path.string();
    } catch (const std::exception& e) {
        std::cerr << "[多模态演示] 生成JPEG小图时出错: " << e.what() << std::endl;
        return input_path;
    }
}

void showMenu() {
    std::cout << "\n=== Mirix 多模态记忆系统 ===" << std::endl;
    std::cout << "1. 基本多模态捕获演示" << std::endl;
    std::cout << "2. 多模态数据处理演示" << std::endl;
    std::cout << "3. 持续多模态捕获演示" << std::endl;
    std::cout << "4. 多模态记忆对话演示" << std::endl;
    std::cout << "5. 配置设置" << std::endl;
    std::cout << "6. 自动多模态记忆对话演示（脚本模式）" << std::endl;
    std::cout << "7. 退出" << std::endl;
}

void demonstrateBasicCapture() {
    LogFormatter::printTitle("基本多模态捕获演示");
    
    if (!g_storage) {
        LogFormatter::printError("存储管理器未初始化");
        return;
    }
    
    // 初始化模型管理器，用于生成多模态描述
    ModelManager modelManager(*g_storage);
    
    // 初始化多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 初始化各个捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 演示图像捕获
    LogFormatter::printStage("步骤 1", "演示图像捕获");
    std::string image_path = "../captures/image_" + std::to_string(time(nullptr)) + ".jpg";
    if (cameraManager.capture_image(image_path)) {
        LogFormatter::printSuccess("图像捕获成功: " + image_path);
        
        // 调用多模态模型生成对图像的真实描述
        std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
        std::string image_description = modelManager.process_multimodal(image_prompt, image_path, "");
        
        // 存储到多模态数据管理器，使用模型输出作为主描述
        if (multimodalManager.saveMultimodalData(1, 1, "image", image_path, image_description)) {
            LogFormatter::printSuccess("图像数据已存储到多模态数据库");
        } else {
            LogFormatter::printError("图像数据存储失败");
        }
    } else {
        LogFormatter::printError("图像捕获失败");
    }
    
    // 演示音频捕获
    LogFormatter::printStage("步骤 2", "演示音频捕获");
    std::string audio_path = "../captures/audio_" + std::to_string(time(nullptr)) + ".wav";
    audioManager.startCapture(audio_path, g_audio_capture_duration);
    // 假设音频捕获成功
    LogFormatter::printSuccess("音频捕获成功: " + audio_path);
    // 存储到多模态数据管理器
    if (multimodalManager.saveMultimodalData(1, 1, "audio", audio_path, "演示过程中录制的环境音/讲解语音，用于还原当时的听觉场景", "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}")) {
        LogFormatter::printSuccess("音频数据已存储到多模态数据库");
    } else {
        LogFormatter::printError("音频数据存储失败");
    }
    
    // 演示屏幕截图
    LogFormatter::printStage("步骤 3", "演示屏幕截图");
    std::string screenshot_path = "../captures/screenshot_" + std::to_string(time(nullptr)) + ".jpg";
    if (screenTracker.capture_screenshot(screenshot_path)) {
        // 生成用于多模态模型的JPEG小图
        std::string small_screenshot_path = create_resized_jpeg_for_multimodal(screenshot_path, "screenshot_basic");
        LogFormatter::printSuccess("屏幕截图成功: " + screenshot_path);
        LogFormatter::printInfo("用于多模态模型的压缩图片: " + small_screenshot_path);

        // 调用多模态模型生成对屏幕截图的真实描述
        std::string screenshot_prompt = "请仔细观察这张屏幕截图，详细描述你看到的内容、界面元素和文字信息。";
        std::string screenshot_description = modelManager.process_multimodal(screenshot_prompt, small_screenshot_path, "");

        // 存储到多模态数据管理器，使用模型输出作为主描述
        if (multimodalManager.saveMultimodalData(1, 1, "screenshot", small_screenshot_path, screenshot_description)) {
            LogFormatter::printSuccess("屏幕截图数据已存储到多模态数据库");
        } else {
            LogFormatter::printError("屏幕截图数据存储失败");
        }
    } else {
        LogFormatter::printError("屏幕截图失败");
    }
    
    LogFormatter::printDoubleSeparator();
    LogFormatter::printSuccess("基本多模态捕获演示完成!");
    
    // 停止任何正在进行的捕获
    cameraManager.stop_capture();
    screenTracker.stop_tracking();
}

void demonstrateMultimodalProcessing() {
    LogFormatter::printTitle("多模态数据处理演示");
    
    if (!g_storage) {
        LogFormatter::printError("存储管理器未初始化");
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
    std::string screenshot_path = "../captures/screenshot_" + std::to_string(time(nullptr)) + ".jpg";
    
    bool capture_success = true;
    
    // 捕获图像
    if (!cameraManager.capture_image(image_path)) {
        LogFormatter::printError("图像捕获失败");
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
        // 先为屏幕截图生成JPEG小图，供后续多模态模型使用
        std::string screenshot_for_llm = create_resized_jpeg_for_multimodal(screenshot_path, "screenshot_multimodal");
        
        // 为图像生成描述并存储
        std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
        ModelManager modelManager(*g_storage);
        std::string image_description = modelManager.process_multimodal(image_prompt, image_path, "");
        if (!multimodalManager.saveMultimodalData(1, 1, "image", image_path, image_description)) {
            LogFormatter::printError("图像数据存储失败");
        }
        
        // 存储音频数据（暂时不需要生成语义描述）
        if (!multimodalManager.saveMultimodalData(1, 1, "audio", audio_path, "多模态处理阶段录制的音频片段，用于辅助理解当前场景", "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}")) {
            LogFormatter::printError("音频数据存储失败");
        }
        
        // 调用多模态大模型进行联合理解
        std::string prompt = "请结合图片、屏幕截图和音频进行理解，并用中文简要描述当前场景。";
        // 使用压缩后的屏幕截图作为图像输入
        std::string multimodal_response = modelManager.process_multimodal(prompt, screenshot_for_llm, audio_path);
        LogFormatter::printSeparator();
        LogFormatter::printBox("多模态模型响应\n" + multimodal_response);
        LogFormatter::printSeparator();
        
        // 使用多模态模型输出作为屏幕截图的主描述写入多模态表
        if (!multimodalManager.saveMultimodalData(1, 1, "screenshot", screenshot_for_llm, multimodal_response)) {
            LogFormatter::printError("屏幕截图数据存储失败");
        }
    } else {
        LogFormatter::printError("多模态数据捕获失败，跳过多模态处理演示");
    }
}

// 多模态记忆对话演示：整合多模态输入 + 记忆系统 + ChatAgent
// 为演示准备多模态场景：捕获数据（摄像头/音频/截图）并生成场景理解
// 返回：场景描述字符串（成功时），空字符串（失败时）
std::string prepare_multimodal_scene_for_demo(int user_id, int agent_id) {
    if (!g_storage) {
        LogFormatter::printError("存储管理器未初始化");
        return "";
    }
    
    // 初始化多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 初始化各个捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);
    
    // === 阶段1：多模态数据捕获 ===
    LogFormatter::printStage("阶段 1", "多模态数据捕获");
    
    std::string image_path = "../captures/image_" + std::to_string(time(nullptr)) + ".jpg";
    std::string audio_path = "../captures/audio_" + std::to_string(time(nullptr)) + ".wav";
    std::string screenshot_path = "../captures/screenshot_" + std::to_string(time(nullptr)) + ".jpg";
    
    bool capture_success = true;
    
    // 捕获摄像头图像
    if (!cameraManager.capture_image(image_path)) {
        LogFormatter::printWarning("摄像头捕获跳过（可能设备不可用）");
    }
    
    // 捕获音频
    LogFormatter::printInfo("开始录制音频 (" + std::to_string(g_audio_capture_duration) + "秒)...");
    if (!audioManager.startCapture(audio_path, g_audio_capture_duration)) {
        LogFormatter::printWarning("音频捕获跳过（可能设备不可用）");
    }
    
    // 捕获屏幕截图
    if (!screenTracker.capture_screenshot(screenshot_path)) {
        LogFormatter::printError("屏幕截图捕获失败");
        capture_success = false;
    }
    
    if (!capture_success) {
        LogFormatter::printError("多模态数据捕获失败");
        return "";
    }
    
    // === 阶段2：生成JPEG小图并存储到资源记忆 ===
    LogFormatter::printStage("阶段 2", "生成JPEG小图并存储资源");
    std::string screenshot_for_llm = create_resized_jpeg_for_multimodal(screenshot_path, "screenshot_chat");
    
    // 初始化模型管理器，用于生成图像和场景描述
    ModelManager modelManager(*g_storage);
    
    // 为图像生成描述并保存到多模态数据表
    std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
    std::string image_description = modelManager.process_multimodal(image_prompt, image_path, "");
    multimodalManager.saveMultimodalData(user_id, agent_id, "image", image_path, image_description);
    
    // 保存音频到多模态数据表（暂时不生成语义描述）
    multimodalManager.saveMultimodalData(user_id, agent_id, "audio", audio_path, "与当前对话场景相关的环境音或语音录制", 
        "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}");
    
    // === 阶段3：调用多模态模型生成场景描述 ===
    LogFormatter::printStage("阶段 3", "调用多模态模型理解当前场景");
    std::string scene_prompt = "请仔细观察这张屏幕截图，详细描述你看到的内容、界面元素、文字信息等。";
    std::string scene_description = modelManager.process_multimodal(scene_prompt, screenshot_for_llm, "");
    
    LogFormatter::printSeparator();
    LogFormatter::printBox("多模态场景理解结果\n" + scene_description);
    LogFormatter::printSeparator();
    
    // 使用模型生成的场景描述作为屏幕截图的主描述写入多模态表
    multimodalManager.saveMultimodalData(user_id, agent_id, "screenshot", screenshot_for_llm, scene_description);
    
    return scene_description;
}

void demonstrateMultimodalMemoryChat() {
    std::cout << "\n=== 多模态记忆对话演示 ===" << std::endl;
    std::cout << "本演示将整合：多模态捕获 -> 场景理解 -> 记忆存储 -> 智能对话" << std::endl;
    std::cout << "演示步骤：" << std::endl;
    std::cout << "  1) 捕获当前屏幕/摄像头/音频，生成多模态记录" << std::endl;
    std::cout << "  2) 调用多模态模型理解场景，并写入多模态记忆与对话记忆" << std::endl;
    std::cout << "  3) 与 AI 对话几轮，让系统根据当前场景形成长期记忆" << std::endl;
    std::cout << "  4) 最后可以提问：‘你刚才看到我在做什么？’、‘请总结刚才的场景’ 等回顾类问题" << std::endl;
    
    if (!g_storage) {
        std::cerr << "存储管理器未初始化" << std::endl;
        return;
    }
    
    // 创建或获取用于多模态记忆对话演示的用户
    int user_id = UserManager::getUserIdByName("DemoUser", g_storage->getDatabasePath());
    if (user_id <= 0) {
        if (UserManager::addUser("DemoUser", "demo@mirix.local", "", g_storage->getDatabasePath())) {
            user_id = UserManager::getUserIdByName("DemoUser", g_storage->getDatabasePath());
            LogFormatter::printSuccess("创建演示用户成功，ID: " + std::to_string(user_id));
        } else {
            LogFormatter::printError("创建用户失败");
            return;
        }
    } else {
        LogFormatter::printInfo("使用已存在的用户，ID: " + std::to_string(user_id));
    }
    int agent_id = 1; // 使用默认Agent ID
    
    // 初始化多模态数据管理器，用于查询最近的场景记录
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
    // 首先查询最近的多模态记忆，判断是否有近期的场景描述可以复用
    auto recent_multimodals = multimodalManager.getMultimodalDataByUserId(user_id, 5);
    std::string scene_description;
    bool reuse_recent_scene = false;
    
    // 如果最近有5分钟内有screenshot类型的记录，提示用户是否复用
    for (const auto& mm : recent_multimodals) {
        if (mm.data_type == "screenshot" && !mm.description.empty()) {
            // 简单的时间检查：判断是否在近期（这里简化为只查询最近的记录）
            LogFormatter::printInfo("检测到最近的场景记录：" + mm.description.substr(0, std::min<size_t>(100, mm.description.size())) + "...");
            std::cout << "是否复用此场景（跳过重新捕获）？ (y/n): ";
            char choice;
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                scene_description = mm.description;
                reuse_recent_scene = true;
                LogFormatter::printSuccess("复用最近的场景描述");
            }
            break;
        }
    }
    
    // 如果不复用，则执行阶段1-3：捕获多模态数据并生成场景理解
    if (!reuse_recent_scene) {
        scene_description = prepare_multimodal_scene_for_demo(user_id, agent_id);
        if (scene_description.empty()) {
            LogFormatter::printError("多模态场景准备失败，演示终止");
            return;
        }
    }
    
    // === 阶段4：初始化ChatAgent和记忆系统 ===
    LogFormatter::printStage("阶段 4", "初始化ChatAgent和记忆系统");
    
    // 创建会话
    SessionManager sessionManager(*g_storage);
    int session_id = sessionManager.create_session(user_id, "{}");
    if (session_id <= 0) {
        LogFormatter::printError("创建会话失败");
        return;
    }
    LogFormatter::printSuccess("创建新会话，ID: " + std::to_string(session_id));
    
    // 初始化 MemoryOrchestrator 和 ChatAgent
    auto orchestrator = std::make_shared<MemoryOrchestratorImpl>(g_storage->getORMWrapper());
    ChatAgent chatAgent(orchestrator);
    chatAgent.initialize(user_id, agent_id);
    
    // === 阶段5：将场景描述作为系统消息存入MessageMemory ===
    LogFormatter::printStage("阶段 5", "将场景描述存入对话记忆");
    auto& messageManager = orchestrator->getMessageMemoryManager();
    MessageMemory scene_message;
    scene_message.user_id = agent_id; // 来自系统/Agent
    scene_message.agent_id = user_id;
    scene_message.session_id = session_id;
    scene_message.content = "[系统] 当前场景：" + scene_description;
    scene_message.created_at = TimeUtils::getCurrentTimestamp();
    messageManager.add(scene_message);
    LogFormatter::printSuccess("场景描述已存入MessageMemory");
    
    // 注：记忆生成由 ChatAgent 中的 MemoryGenerationScheduler 异步调度
    // 不需要在 demo 中手动触发，避免重复触发
    
    // === 阶段6：与ChatAgent进行对话 ===
    LogFormatter::printStage("阶段 6", "开始多模态记忆对话");
    LogFormatter::printBox("您可以基于刚才捕获的场景与AI对话。\n"
                           "输入 'quit' 退出对话。\n"
                           "建议先问：\n"
                           "  - '请总结刚才的屏幕场景'\n"
                           "  - '你刚才看到我在做什么？'\n"
                           "然后再自由提问。");
        
    std::string user_input;
    // 注意：只在第一次进入对话循环时清除缓冲区
    // 如果前面有使用 cin >> 读取数字，需要 ignore 清除残留换行符
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
    while (true) {
        // 打印用户提示符，光标停在提示符后等待输入
        std::cout << "\n" << LogFormatter::Color::BRIGHT_CYAN << LogFormatter::Color::BOLD 
                  << "[用户] > " << LogFormatter::Color::RESET << LogFormatter::Color::WHITE;
        std::cout.flush();  // 确保提示符立即显示
        
        std::getline(std::cin, user_input);
        std::cout << LogFormatter::Color::RESET;  // 重置颜色
            
        if (user_input == "quit" || user_input == "exit") {
            LogFormatter::printInfo("结束对话。");
            break;
        }
            
        if (user_input.empty()) {
            continue;
        }
            
        // 调用ChatAgent处理对话
        std::string ai_response = chatAgent.chat(user_id, agent_id, session_id, user_input);
        LogFormatter::printAIMessage(ai_response);
    }
    
    // 结束会话
    sessionManager.end_session(session_id);

    // === 附加展示：打印记忆生成摘要 ===
    LogFormatter::printSeparator();
    std::cout << "\n" << LogFormatter::Color::BRIGHT_YELLOW << "[记忆系统摘要]" << LogFormatter::Color::RESET << std::endl;
    try {
        auto& episodicManager = orchestrator->getEpisodicMemoryManager();
        auto& semanticManager = orchestrator->getSemanticMemoryManager();
        auto& coreManager = orchestrator->getCoreMemoryBlockManager();
        auto& proceduralManager = orchestrator->getProceduralMemoryManager();

        // 统计本次会话生成的各类长期记忆数量
        auto episodics = episodicManager.getBySessionId(session_id);
        auto semantics = semanticManager.getByUserId(user_id);
        auto cores = coreManager.getByUserId(user_id);
        auto procedurals = proceduralManager.getByUserId(user_id);
        
        std::cout << "  本次对话生成了:" << std::endl;
        std::cout << "    - " << episodics.size() << " 条情景记忆" << std::endl;
        std::cout << "    - " << semantics.size() << " 条语义记忆" << std::endl;
        std::cout << "    - " << cores.size() << " 条核心记忆" << std::endl;
        std::cout << "    - " << procedurals.size() << " 条程序记忆" << std::endl;
        
        // 提示用户如何查看详细信息
        if (episodics.size() > 0 || semantics.size() > 0) {
            std::cout << "\n  提示：使用 CLI 命令查看详细信息：" << std::endl;
            std::cout << "    - memory list episodic  # 查看情景记忆" << std::endl;
            std::cout << "    - memory list semantic  # 查看语义记忆" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[记忆摘要] 获取记忆统计信息时出错: " << e.what() << std::endl;
    }

    std::cout << "\n多模态记忆对话演示结束。" << std::endl;
}

void demonstrateMultimodalMemoryChatAuto() {
    std::cout << "\n=== 多模态记忆对话自动演示 ===" << std::endl;
    std::cout << "本模式将自动完成：多模态捕获 -> 场景理解 -> 记忆存储 -> 若干轮典型问答" << std::endl;

    if (!g_storage) {
        std::cerr << "存储管理器未初始化" << std::endl;
        return;
    }

    // 创建或获取用于自动多模态记忆演示的用户
    int user_id = UserManager::getUserIdByName("DemoUserAuto", g_storage->getDatabasePath());
    if (user_id <= 0) {
        if (UserManager::addUser("DemoUserAuto", "demo-auto@mirix.local", "", g_storage->getDatabasePath())) {
            user_id = UserManager::getUserIdByName("DemoUserAuto", g_storage->getDatabasePath());
            std::cout << "创建自动演示用户成功，ID: " << user_id << std::endl;
        } else {
            std::cerr << "创建自动演示用户失败" << std::endl;
            return;
        }
    } else {
        std::cout << "使用已存在的自动演示用户，ID: " << user_id << std::endl;
    }
    int agent_id = 1;

    // 为了避免重复实现，直接重用互动版本中的核心流程
    // 1) 捕获与理解当前场景
    MultimodalDataManager multimodalManager("../database/mirix.db");
    CameraCaptureManager cameraManager(*g_storage);
    AudioCaptureManager audioManager(*g_storage);
    ScreenActivityTracker screenTracker(*g_storage);

    std::string image_path = "../captures/auto_image_" + std::to_string(time(nullptr)) + ".jpg";
    std::string audio_path = "../captures/auto_audio_" + std::to_string(time(nullptr)) + ".wav";
    std::string screenshot_path = "../captures/auto_screenshot_" + std::to_string(time(nullptr)) + ".jpg";

    bool capture_success = true;
    if (!cameraManager.capture_image(image_path)) {
        std::cout << "摄像头捕获跳过（可能设备不可用）" << std::endl;
    }
    std::cout << "开始录制音频 (" << g_audio_capture_duration << "秒)..." << std::endl;
    if (!audioManager.startCapture(audio_path, g_audio_capture_duration)) {
        std::cout << "音频捕获跳过（可能设备不可用）" << std::endl;
    }
    if (!screenTracker.capture_screenshot(screenshot_path)) {
        std::cout << "屏幕截图捕获失败" << std::endl;
        capture_success = false;
    }

    if (!capture_success) {
        std::cout << "多模态数据捕获失败，自动演示终止" << std::endl;
        return;
    }

    // 2) 压缩截图并生成场景描述
    std::string screenshot_for_llm = create_resized_jpeg_for_multimodal(screenshot_path, "screenshot_chat_auto");
    ModelManager modelManager(*g_storage);

    std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
    std::string image_description = modelManager.process_multimodal(image_prompt, image_path, "");
    multimodalManager.saveMultimodalData(user_id, agent_id, "image", image_path, image_description);

    multimodalManager.saveMultimodalData(user_id, agent_id, "audio", audio_path, "与当前对话场景相关的环境音或语音录制",
        "{\"duration\": " + std::to_string(g_audio_capture_duration) + "}");

    std::string scene_prompt = "请仔细观察这张屏幕截图，详细描述你看到的内容、界面元素、文字信息等。";
    std::string scene_description = modelManager.process_multimodal(scene_prompt, screenshot_for_llm, "");

    std::cout << "\n=== 自动模式：多模态场景理解结果 ===" << std::endl;
    std::cout << scene_description << std::endl;
    std::cout << "===========================\n" << std::endl;

    multimodalManager.saveMultimodalData(user_id, agent_id, "screenshot", screenshot_for_llm, scene_description);

    // 3) 初始化用户、会话与 ChatAgent
    // （用户和agent已在前面创建，这里仅创建会话）
    SessionManager sessionManager(*g_storage);
    int session_id = sessionManager.create_session(user_id, "{\"mode\":\"auto_demo\"}");
    if (session_id <= 0) {
        std::cerr << "创建会话失败" << std::endl;
        return;
    }
    std::cout << "创建自动演示会话，ID: " << session_id << std::endl;

    auto orchestrator = std::make_shared<MemoryOrchestratorImpl>(g_storage->getORMWrapper());
    ChatAgent chatAgent(orchestrator);
    chatAgent.initialize(user_id, agent_id);

    auto& messageManager = orchestrator->getMessageMemoryManager();
    MessageMemory scene_message;
    scene_message.user_id = agent_id;
    scene_message.agent_id = user_id;
    scene_message.session_id = session_id;
    scene_message.content = "[系统-自动演示] 当前场景：" + scene_description;
    scene_message.created_at = TimeUtils::getCurrentTimestamp();
    messageManager.add(scene_message);

    // 在自动模式下，同样手动触发一次记忆整理，确保后续提问可以命中长期记忆
    std::cout << "\n[自动模式] 为演示触发一次记忆整理..." << std::endl;
    try {
        MemoryGenerationManager memGen(orchestrator);
        memGen.processUnprocessedMessages(user_id);
    } catch (const std::exception& e) {
        std::cerr << "[自动模式] 触发记忆生成失败: " << e.what() << std::endl;
    }

    // 4) 自动脚本对话
    std::cout << "\n[自动对话阶段] 系统将自动提问几轮以展示记忆效果..." << std::endl;
    std::vector<std::string> demo_questions = {
        "请总结刚才的屏幕场景。",
        "根据你看到的内容，猜一猜我刚才可能在做什么？"
    };

    for (const auto& q : demo_questions) {
        std::cout << "\n用户(脚本)> " << q << std::endl;
        std::string ai_response = chatAgent.chat(user_id, agent_id, session_id, q);
        std::cout << "AI> " << ai_response << std::endl;
    }

    sessionManager.end_session(session_id);
    std::cout << "\n多模态记忆对话自动演示结束。" << std::endl;
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
    
    // 创建多模态模型管理器，用于生成图像描述
    ModelManager modelManager(*g_storage);
    
    // 创建屏幕活动跟踪器
    ScreenActivityTracker screenTracker(*g_storage);
    
    // 创建摄像头捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    
    int captureCount = 0;
    const int maxCaptures = 3;
    
    while (g_is_running && captureCount < maxCaptures) {
        std::cout << "\n--- 第 " << (captureCount + 1) << " 次捕获 ---" << std::endl;
        
        // 捕获屏幕截图
        std::string screenshotPath = "captures/screen/screenshot_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(captureCount) + ".jpg"; // 使用预定义路径
        bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath); // 使用正确的函数名
        if (screenshotSuccess) {
            std::cout << "捕获屏幕截图: " << screenshotPath << std::endl;
            
            // 保存到多模态数据表
            multimodalManager.saveMultimodalData(1, 1, "screenshot", screenshotPath, "屏幕截图");
        }
        
        // 捕获摄像头图像
        std::string imagePath = "captures/camera/image_" + std::to_string(captureCount) + ".jpg"; // 使用预定义路径
        bool captureSuccess = cameraManager.capture_image(imagePath); // 使用正确的函数名
        if (captureSuccess) {
            std::cout << "捕获摄像头图像: " << imagePath << std::endl;
            
            // 为图像生成描述并保存到多模态数据表
            std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
            std::string image_description = modelManager.process_multimodal(image_prompt, imagePath, "");
            multimodalManager.saveMultimodalData(1, 1, "image", imagePath, image_description);
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
    
    // 创建多模态模型管理器，用于生成图像描述
    ModelManager modelManager(*g_storage);
    // multimodalManager.initialize(); // 注释掉不存在的函数调用
    
    // 创建摄像头捕获管理器
    CameraCaptureManager cameraManager(*g_storage);
    
    // 捕获一张图片
    std::string imagePath = "captures/camera/test_image_" + std::to_string(std::time(nullptr)) + ".jpg"; // 使用预定义路径
    bool captureSuccess = cameraManager.capture_image(imagePath); // 使用正确的函数名
    if (captureSuccess) {
        std::cout << "成功捕获图片: " << imagePath << std::endl;
        
        // 创建多模态模型管理器并生成图像描述
        ModelManager modelManager(*g_storage);
        std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
        std::string image_description = modelManager.process_multimodal(image_prompt, imagePath, "");
        
        // 保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "image", imagePath, image_description);
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
    
    // 创建音频捕获管理器
    AudioCaptureManager audioManager(*g_storage);
    
    // 捕获音频
    std::string audioPath = "captures/audio/test_audio.wav"; // 使用预定义路径
    bool captureSuccess = audioManager.startCapture(audioPath, g_audio_capture_duration);
    if (captureSuccess) {
        std::cout << "成功录制音频: " << audioPath << std::endl;
        // 将音频保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "audio", audioPath, "测试场景中录制的一段示例音频");
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
    
    // 创建多模态数据管理器
    MultimodalDataManager multimodalManager("../database/mirix.db");
    
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
        // 保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "audio", audioPath, "连续音频录制", "{\"duration\": " + std::to_string(g_continuous_audio_duration) + "}");
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
    ModelManager modelManager(*g_storage);
    
    // 连续捕获循环
    int captureCount = 0;
    int maxCaptures = g_screenshot_count > 0 ? g_screenshot_count : 5;
    if (maxCaptures > 20) {
        maxCaptures = 20; // 避免单次演示截屏数量过多
    }
    
    while (g_is_running && captureCount < maxCaptures) {
        std::cout << "\n--- 第 " << (captureCount + 1) << " 次连续捕获 ---" << std::endl;
        
        // 捕获屏幕截图
        std::string screenshotPath = "captures/screen/continuous_screenshot_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(captureCount) + ".jpg";
        bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath);
        if (screenshotSuccess) {
            std::cout << "捕获屏幕截图: " << screenshotPath << std::endl;
            multimodalManager.saveMultimodalData(1, 1, "screenshot", screenshotPath, "连续屏幕截图");
        }
        
        // 捕获摄像头图像
        std::string imagePath = "captures/camera/continuous_image_" + std::to_string(captureCount) + ".jpg";
        bool captureSuccess = cameraManager.capture_image(imagePath);
        if (captureSuccess) {
            std::cout << "捕获摄像头图像: " << imagePath << std::endl;
            
            // 为连续摄像头图像生成描述并保存
            std::string image_prompt = "请简要描述这张连续捕获的摄像头图片中的内容和场景。";
            std::string image_description = modelManager.process_multimodal(image_prompt, imagePath, "");
            multimodalManager.saveMultimodalData(1, 1, "image", imagePath, image_description);
        }
        
        // 捕获音频
        std::string audioPath = "captures/audio/continuous_audio_" + std::to_string(captureCount) + ".wav";
        bool audioSuccess = audioManager.startCapture(audioPath, g_continuous_audio_duration);
        if (audioSuccess) {
            std::cout << "录制音频: " << audioPath << std::endl;
            multimodalManager.saveMultimodalData(1, 1, "audio", audioPath, "连续捕获过程中录制的环境音/语音", "{\"duration\": " + std::to_string(g_continuous_audio_duration) + "}");
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
        
        // 创建多模态模型管理器并生成图像描述
        ModelManager modelManager(*g_storage);
        std::string image_prompt = "请仔细观察这张摄像头捕获的图片，简要描述你看到的内容和场景。";
        std::string image_description = modelManager.process_multimodal(image_prompt, imagePath, "");
        
        // 保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "image", imagePath, image_description);
    }
    
    // 捕获音频
    std::string audioPath = "captures/audio/demo_audio_" + std::to_string(std::time(nullptr)) + ".wav";
    bool audioSuccess = audioManager.startCapture(audioPath, 5);
    if (audioSuccess) {
        std::cout << "成功录制音频: " << audioPath << std::endl;
        
        // 保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "audio", audioPath, "演示场景中录制的一段环境音/语音", "{\"duration\": 5}");
    }
    
    // 捕获屏幕截图
    std::string screenshotPath = "captures/screen/demo_screenshot_" + std::to_string(std::time(nullptr)) + ".jpg";
    bool screenshotSuccess = screenTracker.capture_screenshot(screenshotPath);
    if (screenshotSuccess) {
        std::cout << "成功捕获屏幕截图: " << screenshotPath << std::endl;
        
        // 保存到多模态数据表
        multimodalManager.saveMultimodalData(1, 1, "screenshot", screenshotPath, "演示屏幕截图");
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
                demonstrateMultimodalMemoryChat();
                break;
            case 5:
                configureSettings();
                break;
            case 6:
                demonstrateMultimodalMemoryChatAuto();
                break;
            case 7:
                std::cout << "感谢使用Mirix多模态记忆系统!" << std::endl;
                break;
            default:
                std::cout << "无效选择，请重新输入。" << std::endl;
                break;
        }
    } while (choice != 7 && g_is_running);
    
    // 清理全局资源
    if (g_storage) {
        delete g_storage;
        g_storage = nullptr;
    }
    
    return 0;
}