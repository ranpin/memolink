#include "EpisodicMemoryGenerator.h"
#include "../../../models/PromptTemplates.h"
#include "../../../models/QwenAPI.h"
#include "../../../database/managers/EpisodicMemoryManager.h"  // 修复路径
#include <json/json.h>
#include <iostream>

// 实现基类的纯虚函数
std::string EpisodicMemoryGenerator::buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) {
    return PromptTemplates::getEpisodicMemoryGenerationPrompt(messages);
}

bool EpisodicMemoryGenerator::saveToDatabaseImpl(const std::string& jsonResult,
                                              int user_id, int agent_id, int session_id,
                                              MemoryOrchestrator& orchestrator) {
    std::cout << "[EpisodicMemoryGenerator] 开始解析JSON并保存到数据库..." << std::endl;
    
    try {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(jsonResult, root)) {
            std::cerr << "[EpisodicMemoryGenerator] ✗ JSON解析失败: " << reader.getFormattedErrorMessages() << std::endl;
            std::cerr << "[EpisodicMemoryGenerator] 原始JSON: " << jsonResult << std::endl;
            return false;
        }
        
        std::cout << "[EpisodicMemoryGenerator] ✓ JSON解析成功" << std::endl;
        
        // 使用MemoryOrchestrator保存记忆
        auto& episodicManager = orchestrator.getEpisodicMemoryManager();
        
        EpisodicMemory memory;
        memory.user_id = user_id;
        memory.agent_id = agent_id;
        memory.session_id = session_id;
        
        // 根据Prompt模板的JSON字段映射
        memory.event_description = root.get("event_description", "").asString();
        memory.location = root.get("location", "").asString();
        memory.timestamp = root.get("time_context", "").asString();
        
        // 解析参与者数组
        const Json::Value& participants = root["participants"];
        if (participants.isArray()) {
            std::string participants_str;
            for (const auto& participant : participants) {
                participants_str += participant.asString() + "; ";
            }
            memory.participants = participants_str;
        }
        
        memory.importance = static_cast<int>(root.get("importance", 0.0).asFloat() * 100);
        
        // 将tags和emotional_context存入metadata
        Json::Value metadata;
        metadata["emotional_context"] = root.get("emotional_context", "");
        metadata["tags"] = root["tags"];
        Json::StreamWriterBuilder writer;
        memory.metadata = Json::writeString(writer, metadata);
        
        std::cout << "[EpisodicMemoryGenerator] 事件描述: " << memory.event_description << std::endl;
        std::cout << "[EpisodicMemoryGenerator] 地点: " << memory.location << std::endl;
        std::cout << "[EpisodicMemoryGenerator] 时间: " << memory.timestamp << std::endl;
        std::cout << "[EpisodicMemoryGenerator] 参与者: " << memory.participants << std::endl;
        std::cout << "[EpisodicMemoryGenerator] 重要性: " << memory.importance << std::endl;
        
        int64_t result = episodicManager.add(memory);
        
        if (result > 0) {
            std::cout << "[EpisodicMemoryGenerator] ✓ 保存成功，记忆ID: " << result << std::endl;
            return true;
        } else {
            std::cerr << "[EpisodicMemoryGenerator] ✗ 数据库插入失败" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[EpisodicMemoryGenerator] ✗ 异常: " << e.what() << std::endl;
        return false;
    }
}

EpisodicMemoryGenerator::GeneratedEpisodicMemory EpisodicMemoryGenerator::parseGeneratedMemory(const std::string& jsonResult) {
    GeneratedEpisodicMemory memory;
    
    Json::Value root;
    Json::Reader reader;
    
    if (reader.parse(jsonResult, root)) {
        memory.event_summary = root.get("summary", "").asString();  // 使用正确的字段名
        memory.event_details = root.get("timestamp", "").asString();  // 使用正确的字段名
        memory.event_type = root.get("location", "").asString();  // 使用正确的字段名
        
        // 解析参与者
        const Json::Value& participants = root["participants"];
        if (participants.isArray()) {
            for (const auto& participant : participants) {
                memory.participants.push_back(participant.asString());
            }
        }
        
        // 解析上下文标签
        const Json::Value& contextTags = root["context_tags"];
        if (contextTags.isArray()) {
            for (const auto& tag : contextTags) {
                memory.key_points.push_back(tag.asString());  // 使用key_points作为上下文标签
            }
        }
        
        memory.importance = root.get("importance", 0.0f).asFloat();
    }
    
    return memory;
}