#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"
#include "../../models/EmbeddingIndexManager.h"
#include "../initialization/ConfigManager.h"

// 语义记忆管理器类
class SemanticMemoryManager : public MemoryManager<SemanticMemory> {
public:
    explicit SemanticMemoryManager(ORMWrapper& orm) 
        : MemoryManager<SemanticMemory>(orm) {}
    
    // 覆盖 remove 方法以实现级联删除
    void remove(int64_t id) {
        orm_.remove<SemanticMemory>(id);
        EmbeddingIndexManager embeddingMgr;
        std::string db_path = ConfigManager::getInstance().getDatabasePath();
        embeddingMgr.deleteEmbeddingByMemory(static_cast<int>(id), "semantic", db_path);
    }
        
    // 根据类别查询语义记忆
    std::vector<SemanticMemory> getByCategory(const std::string& category) {
        return find(sqlite_orm::where(sqlite_orm::c(&SemanticMemory::category) == category));
    }
    
    // 根据置信度范围查询语义记忆
    std::vector<SemanticMemory> getByConfidenceRange(float minConfidence, float maxConfidence) {
        return find(sqlite_orm::where(sqlite_orm::c(&SemanticMemory::confidence) >= minConfidence and
                                     sqlite_orm::c(&SemanticMemory::confidence) <= maxConfidence));
    }
};