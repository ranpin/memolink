#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"
#include "../../models/EmbeddingIndexManager.h"
#include "../initialization/ConfigManager.h"

// 核心记忆管理器类
class CoreMemoryBlockManager : public MemoryManager<CoreMemory> {
public:
    explicit CoreMemoryBlockManager(ORMWrapper& orm) 
        : MemoryManager<CoreMemory>(orm) {}
    
    // 覆盖 remove 方法以实现级联删除
    void remove(int64_t id) {
        orm_.remove<CoreMemory>(id);
        EmbeddingIndexManager embeddingMgr;
        std::string db_path = ConfigManager::getInstance().getDatabasePath();
        embeddingMgr.deleteEmbeddingByMemory(static_cast<int>(id), "core", db_path);
    }
        
    // 根据记忆类型查询核心记忆
    std::vector<CoreMemory> getByMemoryType(const std::string& memoryType) {
        return find(sqlite_orm::where(sqlite_orm::c(&CoreMemory::memory_type) == memoryType));
    }
    
    // 根据优先级查询核心记忆
    std::vector<CoreMemory> getByPriority(int priority) {
        return find(sqlite_orm::where(sqlite_orm::c(&CoreMemory::priority) == priority));
    }
};