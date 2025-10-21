#include <iostream>
#include <vector>
#include "capture/MultimodalDataManager.h"
#include "database/core/StorageManager.h"
#include "capture/ScreenActivityTracker.h" // 包含 ActivityData 定义

void runMultimodalDemo() {
    std::cout << "=== 多模态数据管理演示 ===" << std::endl;
    
    // 初始化存储管理器
    StorageManager storage("../database/mirix.db");
    
    // 创建用户 (假设用户ID为1)
    int userId = 1; 

    // 初始化多模态数据管理器
    MultimodalDataManager dataManager("../database/mirix.db");

    // 模拟添加一些测试数据
    // 添加图片数据
    dataManager.saveMultimodalData(userId, "image", "/path/to/image.jpg", "测试图片数据");

    // 添加音频数据
    dataManager.saveMultimodalData(userId, "audio", "/path/to/audio.wav", "测试音频数据", "", "", {}, "{\"duration\": 120}");

    // 添加屏幕活动数据
    ActivityData activityData;
    activityData.app_name = "测试应用";
    activityData.window_title = "测试窗口";
    activityData.duration_seconds = 300;
    
    // 添加系统事件数据
    dataManager.saveMultimodalData(userId, "system_event", "", "系统事件: 键盘输入", "", "", {}, "{\"event_type\": \"键盘输入\", \"event_data\": \"用户按下了A键\"}");

    std::cout << "测试数据添加完成!" << std::endl;

    // 查询并显示所有多模态数据
    std::cout << "\n=== 查询所有多模态数据 ===" << std::endl;
    auto allData = dataManager.getMultimodalDataByUserId(userId);
    for (const auto& item : allData) {
        std::cout << "ID: " << item.id 
                  << ", 用户ID: " << item.userId 
                  << ", 类型: " << item.dataType 
                  << ", 描述: " << item.description 
                  << ", 时间: " << item.timestamp << std::endl;
    }

    // 搜索多模态数据
    std::cout << "\n=== 搜索多模态数据 ===" << std::endl;
    auto searchData = dataManager.searchMultimodalData(userId, "测试");
    for (const auto& item : searchData) {
        std::cout << "ID: " << item.id 
                  << ", 用户ID: " << item.userId 
                  << ", 类型: " << item.dataType 
                  << ", 描述: " << item.description 
                  << ", 时间: " << item.timestamp << std::endl;
    }

    std::cout << "\n多模态数据管理演示完成!" << std::endl;
}