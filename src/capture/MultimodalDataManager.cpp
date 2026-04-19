#include "MultimodalDataManager.h"
#include "../utils/TimeUtils.h"
#include "../utils/VectorSerializer.h"
#include "../database/core/ORMWrapper.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

MultimodalDataManager::MultimodalDataManager(const std::string& dbPath) : storage_(dbPath) {
    // 检查数据库路径是否有效
    if (dbPath.empty()) {
        std::cerr << "Error: Database path is empty" << std::endl;
        return;
    }
    
    // 不再需要显式初始化StorageManager，ORMWrapper会自动处理
    
    // 初始化CLIP处理器
    clipProcessor_ = std::make_unique<CLIPProcessor>();
}

MultimodalDataManager::~MultimodalDataManager() = default;

bool MultimodalDataManager::createTableIfNotExists() {
    // 表结构由ORM自动创建，无需手动创建
    return true;
}

bool MultimodalDataManager::saveMultimodalData(int userId, int sessionId, const std::string& dataType, 
                                              const std::string& filePath, const std::string& description,
                                              const std::string& metadata) {
    try {
        MultimodalMemory item;
        item.user_id = userId;
        item.session_id = sessionId;
        item.data_type = dataType;
        item.file_path = filePath;
        item.description = description;
        item.metadata = metadata;
        item.created_at = TimeUtils::getCurrentTimestamp();
        
        int id = storage_.insertMultimodalMemory(item);
        return id > 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save multimodal data: " << e.what() << std::endl;
        return false;
    }
}

std::vector<MultimodalMemory> MultimodalDataManager::getMultimodalDataByUserId(int userId, int limit) {
    try {
        return storage_.getMultimodalMemoriesByUserId(userId, limit);
    } catch (const std::exception& e) {
        std::cerr << "Failed to get multimodal data: " << e.what() << std::endl;
        return {};
    }
}



bool MultimodalDataManager::deleteMultimodalData(int id) {
    try {
        return storage_.deleteMultimodalMemory(id);
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete multimodal data: " << e.what() << std::endl;
        return false;
    }
}