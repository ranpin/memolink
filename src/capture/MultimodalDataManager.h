#pragma once

#include "CameraCaptureManager.h"
#include "AudioCaptureManager.h"
#include "ScreenActivityTracker.h"
#include "SystemEventManager.h"
#include "../database/core/StorageManager.h"
#include "../models/CLIPProcessor.h"
#include <string>
#include <memory>

// 前向声明
class ModelManager;

struct MultimodalDataItem {
    int id;
    int userId;
    std::string dataType;  // image, audio, screenshot
    std::string filePath;
    std::string description;  // AI生成的描述
    std::string emotions;     // 情绪分析结果
    std::string transcription; // 音频转录结果
    std::vector<float> embedding; // 特征向量
    std::string timestamp;
    std::string metadata;
};

class MultimodalDataManager {
public:
    MultimodalDataManager(const std::string& dbPath = "../database/mirix.db");
    ~MultimodalDataManager();

    bool saveMultimodalData(int userId, const std::string& dataType, 
                           const std::string& filePath, const std::string& description = "",
                           const std::string& emotions = "", const std::string& transcription = "",
                           const std::vector<float>& embedding = {}, const std::string& metadata = "");

    std::vector<MultimodalDataItem> getMultimodalDataByUserId(int userId, int limit = 50);
    std::vector<MultimodalDataItem> searchMultimodalData(int userId, const std::string& query, int limit = 10);
    
    // 添加多模态向量处理相关方法
    bool saveMultimodalEmbedding(int dataId, const std::vector<float>& embedding);
    std::vector<MultimodalDataItem> searchMultimodalByEmbedding(const std::vector<float>& queryEmbedding, int limit = 10);
    std::vector<float> generateTextEmbedding(const std::string& text);
    std::vector<float> generateImageEmbedding(const std::string& imagePath);
    std::vector<float> generateAudioEmbedding(const std::string& audioPath);
    
    // 删除多模态数据
    bool deleteMultimodalData(int id);

private:
    StorageManager storage_;
    std::unique_ptr<CLIPProcessor> clipProcessor_;
    bool createTableIfNotExists();
    MultimodalDataItem rowToMultimodalDataItem(sqlite3_stmt* stmt);
};