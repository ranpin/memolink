#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include "database/core/StorageManager.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/RecallMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "models/ModelManager.h"
#include "agents/ChatAgent.h"
#include "capture/CameraCaptureManager.h"
#include "capture/AudioCaptureManager.h"
#include "capture/ScreenActivityTracker.h"
#include "utils/TimeUtils.h"
#include "database/initialization/InitDB.h"

class EnhancedComprehensiveDemo {
private:
    StorageManager storage_;
    ModelManager model_;
    std::unique_ptr<ChatAgent> chat_agent_;
    std::unique_ptr<RecallMemoryManager> recall_manager_;
    std::unique_ptr<ResourceMemoryManager> resource_manager_;
    
    int user_id_;
    int session_id_;
    int agent_id_;

public:
    EnhancedComprehensiveDemo() : 
        storage_("./demo.db"),  // 使用已初始化的数据库
        model_(storage_),
        user_id_(-1),
        session_id_(-1),
        agent_id_(-1) {
    }
    
    bool initialize() {
        std::cout << "=== MIRIX 增强综合演示程序 ===" << std::endl;
        std::cout << "初始化增强综合演示..." << std::endl;
        
        if (!storage_.initialize()) {
            std::cerr << "存储管理器初始化失败" << std::endl;
            return false;
        }
        
        // 初始化数据库表
        if (!InitDB::initializeDatabase("./demo.db")) {
            std::cerr << "数据库表初始化失败" << std::endl;
            return false;
        }
        
        // 创建用户（使用空字符串作为password_hash）
        user_id_ = UserManager::addUser("enhanced_demo_user", "enhanced_demo@example.com", "", "./demo.db");
        if (user_id_ <= 0) {
            std::cerr << "创建用户失败" << std::endl;
            return false;
        }
        std::cout << "创建用户成功，用户ID: " << user_id_ << std::endl;
        
        // 创建会话
        SessionManager session_manager(storage_);
        SessionItem session_item;  // 直接使用SessionItem而不是SessionManager::SessionItem
        session_item.user_id = std::to_string(user_id_);
        session_item.status = "active";
        session_item.start_time = TimeUtils::getCurrentTimestamp();
        
        if (!session_manager.create_session(session_item)) {
            std::cerr << "创建会话失败" << std::endl;
            return false;
        }
        
        // 获取会话ID
        auto sessions = session_manager.list_sessions(std::to_string(user_id_));
        if (!sessions.empty()) {
            session_id_ = sessions.back().id;
        } else {
            session_id_ = 1; // 备用方案
        }
        std::cout << "创建会话成功，会话ID: " << session_id_ << std::endl;
        
        // 设置代理ID
        agent_id_ = 1;
        
        // 初始化聊天代理
        chat_agent_ = std::make_unique<ChatAgent>(storage_);
        chat_agent_->initialize(user_id_, agent_id_);
        
        // 初始化回忆记忆和资源记忆管理器
        recall_manager_ = std::make_unique<RecallMemoryManager>(storage_);
        resource_manager_ = std::make_unique<ResourceMemoryManager>(storage_);
        
        std::cout << "增强综合演示初始化完成" << std::endl;
        return true;
    }
    
    void run_demo() {
        std::cout << "\n=== 开始增强综合演示 ===" << std::endl;
        
        // 1. 模拟摄像头捕获
        std::cout << "\n1. 模拟摄像头捕获..." << std::endl;
        std::string image_path = "./captures/camera_demo_" + TimeUtils::getCurrentTimestamp() + ".jpg";
        CameraCaptureManager camera_manager(storage_);
        
        if (camera_manager.capture_image(image_path)) {
            std::cout << "摄像头捕获成功: " << image_path << std::endl;
            
            // 保存到资源记忆管理器
            ResourceMemoryManager::ResourceMemoryItem resource_item;
            resource_item.user_id = user_id_;
            resource_item.agent_id = agent_id_;
            resource_item.title = "摄像头捕获图像";
            resource_item.content = "增强综合演示摄像头捕获";
            resource_item.resource_type = "image";
            resource_item.path = image_path;
            resource_item.metadata = "{}";
            
            if (resource_manager_->add_resource_memory(resource_item)) {
                std::cout << "图像数据保存到资源记忆成功" << std::endl;
                
                // 获取刚插入的资源ID
                auto resources = resource_manager_->list_resource_memories(user_id_);
                if (!resources.empty()) {
                    int resource_id = resources.back().id;
                    
                    // 生成并保存图像embedding
                    std::vector<float> image_embedding = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f}; // 模拟生成
                    std::cout << "生成图像embedding，维度: " << image_embedding.size() << std::endl;
                    
                    // 保存embedding到数据库
                    if (resource_manager_->saveEmbedding(resource_id, image_embedding)) {
                        std::cout << "图像embedding保存成功" << std::endl;
                        
                        // 验证embedding是否正确保存
                        auto retrieved_embedding = resource_manager_->getEmbedding(resource_id);
                        if (!retrieved_embedding.empty()) {
                            std::cout << "验证图像embedding检索成功，维度: " << retrieved_embedding.size() << std::endl;
                        } else {
                            std::cerr << "图像embedding检索失败" << std::endl;
                        }
                    } else {
                        std::cerr << "图像embedding保存失败" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "摄像头捕获失败" << std::endl;
        }
        
        // 2. 模拟音频捕获
        std::cout << "\n2. 模拟音频捕获..." << std::endl;
        std::string audio_path = "./captures/audio_demo_" + TimeUtils::getCurrentTimestamp() + ".wav";
        AudioCaptureManager audio_manager(storage_);
        
        if (audio_manager.startCapture(audio_path, 3)) { // 捕获3秒
            std::cout << "音频捕获成功: " << audio_path << std::endl;
            
            // 保存到资源记忆管理器
            ResourceMemoryManager::ResourceMemoryItem resource_item;
            resource_item.user_id = user_id_;
            resource_item.agent_id = agent_id_;
            resource_item.title = "音频捕获";
            resource_item.content = "增强综合演示音频捕获";
            resource_item.resource_type = "audio";
            resource_item.path = audio_path;
            resource_item.metadata = "{}";
            
            if (resource_manager_->add_resource_memory(resource_item)) {
                std::cout << "音频数据保存到资源记忆成功" << std::endl;
                
                // 获取刚插入的资源ID
                auto resources = resource_manager_->list_resource_memories(user_id_);
                if (!resources.empty()) {
                    int resource_id = resources.back().id;
                    
                    // 生成并保存音频embedding
                    std::vector<float> audio_embedding = {0.5f, 0.4f, 0.3f, 0.2f, 0.1f}; // 模拟生成
                    std::cout << "生成音频embedding，维度: " << audio_embedding.size() << std::endl;
                    
                    // 保存embedding到数据库
                    if (resource_manager_->saveEmbedding(resource_id, audio_embedding)) {
                        std::cout << "音频embedding保存成功" << std::endl;
                        
                        // 验证embedding是否正确保存
                        auto retrieved_embedding = resource_manager_->getEmbedding(resource_id);
                        if (!retrieved_embedding.empty()) {
                            std::cout << "验证音频embedding检索成功，维度: " << retrieved_embedding.size() << std::endl;
                        } else {
                            std::cerr << "音频embedding检索失败" << std::endl;
                        }
                    } else {
                        std::cerr << "音频embedding保存失败" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "音频捕获失败" << std::endl;
        }
        
        // 3. 模拟屏幕截图
        std::cout << "\n3. 模拟屏幕截图..." << std::endl;
        std::string screenshot_path = "./captures/screenshot_demo_" + TimeUtils::getCurrentTimestamp() + ".png";
        ScreenActivityTracker screen_tracker(storage_);
        
        if (screen_tracker.capture_screenshot(screenshot_path)) {
            std::cout << "屏幕截图成功: " << screenshot_path << std::endl;
            
            // 保存到资源记忆管理器
            ResourceMemoryManager::ResourceMemoryItem resource_item;
            resource_item.user_id = user_id_;
            resource_item.agent_id = agent_id_;
            resource_item.title = "屏幕截图";
            resource_item.content = "增强综合演示屏幕截图";
            resource_item.resource_type = "screenshot";
            resource_item.path = screenshot_path;
            resource_item.metadata = "{}";
            
            if (resource_manager_->add_resource_memory(resource_item)) {
                std::cout << "屏幕截图数据保存到资源记忆成功" << std::endl;
                
                // 获取刚插入的资源ID
                auto resources = resource_manager_->list_resource_memories(user_id_);
                if (!resources.empty()) {
                    int resource_id = resources.back().id;
                    
                    // 生成并保存截图embedding
                    std::vector<float> screenshot_embedding = {0.2f, 0.4f, 0.6f, 0.8f, 0.1f}; // 模拟生成
                    std::cout << "生成截图embedding，维度: " << screenshot_embedding.size() << std::endl;
                    
                    // 保存embedding到数据库
                    if (resource_manager_->saveEmbedding(resource_id, screenshot_embedding)) {
                        std::cout << "截图embedding保存成功" << std::endl;
                        
                        // 验证embedding是否正确保存
                        auto retrieved_embedding = resource_manager_->getEmbedding(resource_id);
                        if (!retrieved_embedding.empty()) {
                            std::cout << "验证截图embedding检索成功，维度: " << retrieved_embedding.size() << std::endl;
                        } else {
                            std::cerr << "截图embedding检索失败" << std::endl;
                        }
                    } else {
                        std::cerr << "截图embedding保存失败" << std::endl;
                    }
                }
            }
        } else {
            std::cerr << "屏幕截图失败" << std::endl;
        }
        
        // 4. 模拟聊天交互
        std::cout << "\n4. 模拟聊天交互..." << std::endl;
        std::vector<std::string> demo_messages = {
            "你好，这是一个增强综合演示。",
            "我刚刚捕获了一些多模态数据。",
            "请告诉我你对这些数据的看法。"
        };
        
        int message_count = 0;
        for (const auto& message : demo_messages) {
            message_count++;
            std::cout << "\n用户: " << message << std::endl;
            
            // 保存用户消息到回忆记忆
            RecallMemoryManager::RecallMemoryItem recall_item;
            recall_item.user_id = user_id_;
            recall_item.agent_id = agent_id_;
            recall_item.session_id = session_id_;
            recall_item.message_type = "user_message";
            recall_item.content = message;
            recall_item.metadata = "{}";
            
            if (recall_manager_->add_recall_memory(recall_item)) {
                std::cout << "用户消息保存到回忆记忆成功" << std::endl;
                
                // 获取刚插入的消息ID
                auto messages = recall_manager_->list_recall_memories(user_id_);
                if (!messages.empty()) {
                    int message_id = messages.back().id;
                    
                    // 生成并保存文本embedding
                    std::vector<float> text_embedding = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}; // 模拟生成
                    std::cout << "生成文本embedding，维度: " << text_embedding.size() << std::endl;
                    
                    // 保存embedding到数据库
                    if (recall_manager_->saveEmbedding(message_id, text_embedding)) {
                        std::cout << "文本embedding保存成功" << std::endl;
                        
                        // 验证embedding是否正确保存
                        auto retrieved_embedding = recall_manager_->getEmbedding(message_id);
                        if (!retrieved_embedding.empty()) {
                            std::cout << "验证文本embedding检索成功，维度: " << retrieved_embedding.size() << std::endl;
                        } else {
                            std::cerr << "文本embedding检索失败" << std::endl;
                        }
                    } else {
                        std::cerr << "文本embedding保存失败" << std::endl;
                    }
                }
            }
            
            std::string response = chat_agent_->chat(user_id_, agent_id_, session_id_, message);
            std::cout << "AI助手: " << response << std::endl;
            
            // 保存AI响应到回忆记忆
            RecallMemoryManager::RecallMemoryItem response_item;
            response_item.user_id = user_id_;
            response_item.agent_id = agent_id_;
            response_item.session_id = session_id_;
            response_item.message_type = "ai_response";
            response_item.content = response;
            response_item.metadata = "{}";
            
            if (recall_manager_->add_recall_memory(response_item)) {
                std::cout << "AI响应保存到回忆记忆成功" << std::endl;
                
                // 获取刚插入的消息ID
                auto messages = recall_manager_->list_recall_memories(user_id_);
                if (!messages.empty()) {
                    int message_id = messages.back().id;
                    
                    // 生成并保存文本embedding
                    std::vector<float> text_embedding = {0.9f, 0.7f, 0.5f, 0.3f, 0.1f}; // 模拟生成
                    std::cout << "生成AI响应embedding，维度: " << text_embedding.size() << std::endl;
                    
                    // 保存embedding到数据库
                    if (recall_manager_->saveEmbedding(message_id, text_embedding)) {
                        std::cout << "AI响应embedding保存成功" << std::endl;
                        
                        // 验证embedding是否正确保存
                        auto retrieved_embedding = recall_manager_->getEmbedding(message_id);
                        if (!retrieved_embedding.empty()) {
                            std::cout << "验证AI响应embedding检索成功，维度: " << retrieved_embedding.size() << std::endl;
                        } else {
                            std::cerr << "AI响应embedding检索失败" << std::endl;
                        }
                    } else {
                        std::cerr << "AI响应embedding保存失败" << std::endl;
                    }
                }
            }
            
            // 模拟思考时间
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "\n=== 增强综合演示完成 ===" << std::endl;
        
        // 验证数据检索
        std::cout << "\n=== 验证数据检索 ===" << std::endl;
        auto recall_memories = recall_manager_->list_recall_memories(user_id_);
        std::cout << "回忆记忆总数: " << recall_memories.size() << std::endl;
        
        auto resource_memories = resource_manager_->list_resource_memories(user_id_);
        std::cout << "资源记忆总数: " << resource_memories.size() << std::endl;
    }
};

int main() {
    EnhancedComprehensiveDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "演示初始化失败" << std::endl;
        return 1;
    }
    
    demo.run_demo();
    
    return 0;
}