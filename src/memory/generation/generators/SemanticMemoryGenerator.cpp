#include "SemanticMemoryGenerator.h"
#include "../../../models/PromptTemplates.h"
#include "../../../models/QwenAPI.h"
#include "../../../database/managers/SemanticMemoryManager.h"  // 修复路径
#include <json/json.h>
#include <iostream>

// 实现基类的纯虚函数
std::string SemanticMemoryGenerator::buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) {
    return PromptTemplates::getSemanticMemoryGenerationPrompt(messages);
}

bool SemanticMemoryGenerator::saveToDatabaseImpl(const std::string& jsonResult,
                                              int user_id, int agent_id, int session_id,
                                              MemoryOrchestrator& orchestrator) {
    std::cout << "[SemanticMemoryGenerator] 开始解析JSON并保存到数据库..." << std::endl;
    
    try {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(jsonResult, root)) {
            std::cerr << "[SemanticMemoryGenerator] ✗ JSON解析失败: " << reader.getFormattedErrorMessages() << std::endl;
            std::cerr << "[SemanticMemoryGenerator] 原始JSON: " << jsonResult << std::endl;
            return false;
        }
        
        std::cout << "[SemanticMemoryGenerator] ✓ JSON触析成功" << std::endl;
        
        // 使用MemoryOrchestrator保存记忆
        auto& semanticManager = orchestrator.getSemanticMemoryManager();
        
        SemanticMemory memory;
        memory.user_id = user_id;
        memory.agent_id = agent_id;
        
        // 根据Prompt模板的JSON字段映射
        memory.fact = root.get("fact", "").asString();
        memory.category = root.get("category", "").asString();
        memory.confidence = root.get("confidence", 0.0).asFloat();
        
        // 将tags和related_concepts存入metadata
        Json::Value metadata;
        metadata["tags"] = root["tags"];
        metadata["related_concepts"] = root["related_concepts"];
        metadata["relevance_score"] = root.get("relevance_score", 0.0);
        Json::StreamWriterBuilder writer;
        memory.metadata = Json::writeString(writer, metadata);
        
        std::cout << "[SemanticMemoryGenerator] 事实: " << memory.fact << std::endl;
        std::cout << "[SemanticMemoryGenerator] 分类: " << memory.category << std::endl;
        std::cout << "[SemanticMemoryGenerator] 置信度: " << memory.confidence << std::endl;
        
        int64_t result = semanticManager.add(memory);
        
        if (result > 0) {
            std::cout << "[SemanticMemoryGenerator] ✓ 保存成功，记忆ID: " << result << std::endl;
            return true;
        } else {
            std::cerr << "[SemanticMemoryGenerator] ✗ 数据库插入失败" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[SemanticMemoryGenerator] ✗ 异常: " << e.what() << std::endl;
        return false;
    }
}

