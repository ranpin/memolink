#pragma once

#include "CameraCaptureManager.h"
#include "AudioCaptureManager.h"
#include "ScreenActivityTracker.h"
//#include "SystemEventManager.h"
#include "../database/core/ORMWrapper.h"
#include "../models/CLIPProcessor.h"
#include "../database/models/MemoryModels.h"
#include <string>
#include <memory>

// 前向声明
class ModelManager;

// 使用MemoryModels.h中定义的MultimodalMemory

class MultimodalDataManager {
public:
    MultimodalDataManager(const std::string& dbPath = "../database/mirix.db");
    ~MultimodalDataManager();

    bool saveMultimodalData(int userId, int sessionId, const std::string& dataType, 
                           const std::string& filePath, const std::string& description = "",
                           const std::string& metadata = "{}");

    std::vector<MultimodalMemory> getMultimodalDataByUserId(int userId, int limit = 50);
    
    // 添加搜索功能
    std::vector<MultimodalMemory> searchMultimodalData(int userId, const std::string& query, int limit = 50);
    
    // 删除多模态数据
    bool deleteMultimodalData(int id);

private:
    ORMWrapper storage_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;
    bool createTableIfNotExists();
};