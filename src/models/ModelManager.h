#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

// ModelManager - 多模态模型推理管理器
#include <string>
#include <vector>
#include "../database/core/StorageManager.h"
#include "QwenAPI.h"

class ModelManager {
public:
    // 构造函数
    ModelManager(StorageManager& storage);
    
    // 文本模型接口
    std::string generateResponse(const std::vector<std::string>& context, const std::string& query);
    
    // 嵌入模型接口
    std::vector<float> generateEmbedding(const std::string& text);
    
    // 多模态模型接口
    std::string process_multimodal(const std::string& text, 
                                  const std::string& image_path = "",
                                  const std::string& audio_path = "");

private:
    StorageManager& storage_;
    QwenAPI qwen_api;
    
    // 模拟模型调用的私有方法
    std::string call_text_model(const std::string& prompt);
    std::string call_text_model_with_context(const std::vector<std::string>& context, const std::string& prompt);
    std::vector<float> call_embedding_model(const std::string& text);
    std::string call_multimodal_model(const std::string& text,
                                     const std::string& image_path,
                                     const std::string& audio_path);
};

#endif // MODEL_MANAGER_H