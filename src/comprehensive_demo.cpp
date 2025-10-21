#include <iostream>
#include <memory>
#include <string>

#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "models/ModelManager.h"
#include "agents/AgentManager.h"
#include "agents/ChatAgent.h"
#include "capture/MultimodalDataManager.h"
#include "utils/TimeUtils.h"

class ComprehensiveDemo {
private:
    StorageManager storage_;
    std::unique_ptr<ModelManager> model_;
    std::unique_ptr<AgentManager> agent_manager_;
    std::unique_ptr<ChatAgent> chat_agent_;
    std::unique_ptr<MultimodalDataManager> multimodal_manager_;
    int user_id_;
    int default_agent_id_;

public:
    ComprehensiveDemo() : storage_("../database/mirix.db"), multimodal_manager_(std::make_unique<MultimodalDataManager>("../database/mirix.db")) {
        // 初始化模型管理器
        model_ = std::make_unique<ModelManager>(storage_);
        
        // 初始化代理管理器和聊天代理
        agent_manager_ = std::make_unique<AgentManager>(storage_, *model_);
        chat_agent_ = std::make_unique<ChatAgent>(storage_);
        
        // 创建默认agent
        StorageManager agent_storage("../database/mirix.db");
        ModelManager model(agent_storage);
        AgentManager agent_manager(agent_storage, model);
        default_agent_id_ = agent_manager.create_agent("DefaultAgent", "默认AI助手", "一个通用的AI助手");
        
        // 创建默认用户
        user_id_ = 1; // 假设用户ID为1
        
        // 初始化聊天代理
        chat_agent_->initialize(user_id_, default_agent_id_);
        
        std::cout << "综合演示系统初始化完成!" << std::endl;
        std::cout << "默认用户ID: " << user_id_ << std::endl;
        std::cout << "默认Agent ID: " << default_agent_id_ << std::endl;
    }

    void run() {
        std::cout << "\n=== Mirix综合演示系统 ===" << std::endl;
        std::cout << "1. 多模态数据捕获演示" << std::endl;
        std::cout << "2. 记忆系统交互演示" << std::endl;
        std::cout << "3. AI代理对话演示" << std::endl;
        std::cout << "4. 退出" << std::endl;
        
        int choice;
        std::cout << "请选择演示项目: ";
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                runMultimodalDemo();
                break;
            case 2:
                runMemoryDemo();
                break;
            case 3:
                runChatDemo();
                break;
            case 4:
                std::cout << "感谢使用Mirix综合演示系统!" << std::endl;
                break;
            default:
                std::cout << "无效选择，请重新输入。" << std::endl;
                run();
        }
    }
    
private:
    void runMultimodalDemo() {
        std::cout << "\n=== 多模态数据捕获演示 ===" << std::endl;
        
        // 模拟摄像头捕获
        std::string image_path = "/tmp/demo_image.jpg";
        std::cout << "模拟摄像头捕获: " << image_path << std::endl;
        // 这里应该调用实际的摄像头捕获函数
        
        // 模拟音频捕获
        std::string audio_path = "/tmp/demo_audio.wav";
        std::cout << "模拟音频捕获: " << audio_path << std::endl;
        // 这里应该调用实际的音频捕获函数
        
        // 模拟屏幕截图
        std::string screenshot_path = "/tmp/demo_screenshot.png";
        std::cout << "模拟屏幕截图: " << screenshot_path << std::endl;
        // 这里应该调用实际的屏幕截图函数
        
        std::cout << "多模态数据捕获演示完成!" << std::endl;
        run();
    }

    void runMemoryDemo() {
        std::cout << "\n=== 记忆系统交互演示 ===" << std::endl;
        std::cout << "记忆系统交互演示功能待实现..." << std::endl;
        run();
    }

    void runChatDemo() {
        std::cout << "\n=== AI代理对话演示 ===" << std::endl;
        std::cout << "AI代理对话演示功能待实现..." << std::endl;
        run();
    }
};

int main() {
    try {
        ComprehensiveDemo demo;
        demo.run();
    } catch (const std::exception& e) {
        std::cerr << "程序运行出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}