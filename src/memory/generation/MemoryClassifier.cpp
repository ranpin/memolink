#include "MemoryClassifier.h"
#include "../../models/ModelManager.h"
#include <iostream>
#include <json/json.h>

using namespace std;

MemoryClassifier::MemoryTypePrediction MemoryClassifier::classify(
    const vector<MessageManager::Message>& messages) {
    
    cout << "正在对记忆进行分类..." << endl;
    
    // 构建Prompt
    string prompt = buildClassificationPrompt(messages);
    
    // 调用大模型API进行分类
    QwenAPI qwen;
    QwenChatCompletionRequest request;
    request.model = "qwen-plus";
    
    QwenMessage message;
    message.role = "user";
    message.content = prompt;
    request.messages.push_back(message);
    
    QwenChatCompletionResponse response = qwen.chat_completion(request);
    
    // 解析响应
    MemoryTypePrediction prediction = parseClassificationResult(response.content);
    
    cout << "记忆分类完成" << endl;
    return prediction;
}

string MemoryClassifier::buildClassificationPrompt(
    const vector<MessageManager::Message>& messages) {
    
    // 构建提示词
    string prompt = string("请分析对话历史并将其分类为以下类型之一:\n") +
                   "- episodic (情景记忆)\n" +
                   "- semantic (语义记忆)\n" +
                   "- core (核心记忆)\n" +
                   "- procedural (程序记忆)\n" +
                   "- resource (资源记忆)\n" +
                   "- secret (秘密记忆)\n\n" +
                   "请以JSON格式回复，包含以下字段:\n" +
                   "- thought: 你的思考过程\n" +
                   "- episodic: 是否为情景记忆(true/false)\n" +
                   "- semantic: 是否为语义记忆(true/false)\n" +
                   "- core: 是否为核心记忆(true/false)\n" +
                   "- procedural: 是否为程序记忆(true/false)\n" +
                   "- resource: 是否为资源记忆(true/false)\n" +
                   "- secret: 是否为秘密记忆(true/false)\n" +
                   "- episodic_confidence: 情景记忆置信度(0-1)\n" +
                   "- semantic_confidence: 语义记忆置信度(0-1)\n" +
                   "- core_confidence: 核心记忆置信度(0-1)\n" +
                   "- procedural_confidence: 程序记忆置信度(0-1)\n" +
                   "- resource_confidence: 资源记忆置信度(0-1)\n" +
                   "- secret_confidence: 秘密记忆置信度(0-1)";
    
    return prompt;
}

MemoryClassifier::MemoryTypePrediction MemoryClassifier::parseClassificationResult(
    const string& jsonResult) {
    
    MemoryTypePrediction prediction;
    
    // 初始化默认值
    prediction.episodic = false;
    prediction.semantic = false;
    prediction.core = false;
    prediction.procedural = false;
    prediction.resource = false;
    prediction.secret = false;
    prediction.episodic_confidence = 0.0f;
    prediction.semantic_confidence = 0.0f;
    prediction.core_confidence = 0.0f;
    prediction.procedural_confidence = 0.0f;
    prediction.resource_confidence = 0.0f;
    prediction.secret_confidence = 0.0f;
    
    try {
        // 查找JSON内容（去除可能的额外文本）
        size_t firstBrace = jsonResult.find('{');
        size_t lastBrace = jsonResult.rfind('}');
        
        if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace > firstBrace) {
            std::string jsonString = jsonResult.substr(firstBrace, lastBrace - firstBrace + 1);
            
            // 解析JSON
            Json::Value root;
            Json::Reader reader;
            
            if (reader.parse(jsonString, root)) {
                // 解析布尔值
                if (root.isMember("episodic")) {
                    prediction.episodic = root["episodic"].asBool();
                }
                if (root.isMember("semantic")) {
                    prediction.semantic = root["semantic"].asBool();
                }
                if (root.isMember("core")) {
                    prediction.core = root["core"].asBool();
                }
                if (root.isMember("procedural")) {
                    prediction.procedural = root["procedural"].asBool();
                }
                if (root.isMember("resource")) {
                    prediction.resource = root["resource"].asBool();
                }
                if (root.isMember("secret")) {
                    prediction.secret = root["secret"].asBool();
                }
                
                // 解析置信度
                if (root.isMember("confidence") && root["confidence"].isObject()) {
                    Json::Value confidence = root["confidence"];
                    if (confidence.isMember("episodic")) {
                        prediction.episodic_confidence = confidence["episodic"].asFloat();
                    }
                    if (confidence.isMember("semantic")) {
                        prediction.semantic_confidence = confidence["semantic"].asFloat();
                    }
                    if (confidence.isMember("core")) {
                        prediction.core_confidence = confidence["core"].asFloat();
                    }
                    if (confidence.isMember("procedural")) {
                        prediction.procedural_confidence = confidence["procedural"].asFloat();
                    }
                    if (confidence.isMember("resource")) {
                        prediction.resource_confidence = confidence["resource"].asFloat();
                    }
                    if (confidence.isMember("secret")) {
                        prediction.secret_confidence = confidence["secret"].asFloat();
                    }
                }
            } else {
                std::cerr << "[MemoryClassifier] Failed to parse JSON: " << reader.getFormattedErrorMessages() << std::endl;
                std::cerr << "[MemoryClassifier] JSON string was: " << jsonString << std::endl;
            }
        } else {
            std::cerr << "[MemoryClassifier] No valid JSON found in response" << std::endl;
            std::cerr << "[MemoryClassifier] Response was: " << jsonResult << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[MemoryClassifier] Exception during JSON parsing: " << e.what() << std::endl;
    }
    
    return prediction;
}