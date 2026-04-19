#include "MemoryAgent.h"
#include "../database/managers/CoreMemoryBlockManager.h"
#include "../database/managers/SecretVaultManager.h"
#include "../database/managers/MessageMemoryManager.h"
#include "../database/managers/MemoryOrchestratorImpl.h"
#include "../utils/TimeUtils.h"
#include <iostream>

MemoryAgent::MemoryAgent(StorageManager& storage) 
    : storage_(storage) {
    // 创建MemoryOrchestratorImpl实例而非抽象类MemoryOrchestrator
    orchestrator_ = std::make_unique<MemoryOrchestratorImpl>(storage_.getORMWrapper());
}

// 核心记忆操作
bool MemoryAgent::addCoreMemoryBlock(int user_id, const std::string& key, const std::string& value, const std::string& metadata) {
    CoreMemory memory;
    memory.user_id = user_id;
    memory.memory_type = key;
    memory.content = value;
    memory.metadata = metadata;
    auto& manager = orchestrator_->getCoreMemoryBlockManager();
    return manager.add(memory) > 0;
}

std::map<std::string, std::string> MemoryAgent::getCoreMemoryBlock(int user_id) {
    // 获取所有核心记忆块
    auto& manager = orchestrator_->getCoreMemoryBlockManager();
    auto memories = manager.getAll();
    
    // 遍历所有记录，为该用户构建一个结果映射
    std::map<std::string, std::string> result;
    for (const auto& memory : memories) {
        if (memory.user_id == user_id) {
            // 将每个匹配的记忆块作为键值对添加到结果中
            result[memory.memory_type] = memory.content;
        }
    }
    return result; // 如果没有找到，将返回一个空的 map
}

bool MemoryAgent::updateCoreMemoryBlock(int user_id, const std::string& key, const std::string& value, const std::string& metadata) {
    // 查找现有记录
    auto& manager = orchestrator_->getCoreMemoryBlockManager();
    auto memories = manager.getAll();
    for (const auto& memory : memories) {
        if (memory.user_id == user_id && memory.memory_type == key) {
            // 更新记录
            CoreMemory updatedMemory = memory;
            updatedMemory.content = value;
            updatedMemory.metadata = metadata;
            return manager.update(updatedMemory);
        }
    }
    
    // 如果找不到现有记录，则创建新记录
    CoreMemory newMemory;
    newMemory.user_id = user_id;
    newMemory.memory_type = key;
    newMemory.content = value;
    newMemory.metadata = metadata;
    return manager.add(newMemory) > 0;
}

bool MemoryAgent::deleteCoreMemoryBlock(int user_id, const std::string& key) {
    // 查找要删除的记录
    auto& manager = orchestrator_->getCoreMemoryBlockManager();
    auto memories = manager.getAll();
    for (const auto& memory : memories) {
        if (memory.user_id == user_id && memory.memory_type == key) {
            manager.remove(memory.id);
            return true;
        }
    }
    return false;
}

// 语义记忆操作
std::vector<std::map<std::string, std::string>> MemoryAgent::listSemanticMemories(int user_id, int limit) {
    std::vector<std::map<std::string, std::string>> result;
    // 简单实现：返回空列表
    return result;
}