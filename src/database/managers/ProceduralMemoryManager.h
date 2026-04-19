#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"
#include "../../models/EmbeddingIndexManager.h"
#include "../initialization/ConfigManager.h"

// 程序记忆管理器类
class ProceduralMemoryManager : public MemoryManager<ProceduralMemory> {
public:
    explicit ProceduralMemoryManager(ORMWrapper& orm) 
        : MemoryManager<ProceduralMemory>(orm) {}
    
    // 覆盖 remove 方法以实现级联删除
    void remove(int64_t id) {
        orm_.remove<ProceduralMemory>(id);
        EmbeddingIndexManager embeddingMgr;
        std::string db_path = ConfigManager::getInstance().getDatabasePath();
        embeddingMgr.deleteEmbeddingByMemory(static_cast<int>(id), "procedural", db_path);
    }
        
    // 根据名称查询程序记忆
    std::vector<ProceduralMemory> getByName(const std::string& name) {
        return find(sqlite_orm::where(sqlite_orm::c(&ProceduralMemory::name) == name));
    }
};