#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"
#include "../../models/EmbeddingIndexManager.h"
#include "../initialization/ConfigManager.h"

// 事件记忆管理器类
class EpisodicMemoryManager : public MemoryManager<EpisodicMemory> {
public:
    explicit EpisodicMemoryManager(ORMWrapper& orm) 
        : MemoryManager<EpisodicMemory>(orm) {}
    
    // 覆盖 remove 方法以实现级联删除
    void remove(int64_t id) {
        // 先删除记忆记录
        orm_.remove<EpisodicMemory>(id);
        
        // 级联删除 embedding 索引
        EmbeddingIndexManager embeddingMgr;
        std::string db_path = ConfigManager::getInstance().getDatabasePath();
        embeddingMgr.deleteEmbeddingByMemory(static_cast<int>(id), "episodic", db_path);
    }
        
    // 根据会话 ID查询事件记忆
    std::vector<EpisodicMemory> getBySessionId(int64_t sessionId) {
        return find(sqlite_orm::where(sqlite_orm::c(&EpisodicMemory::session_id) == sessionId));
    }
    
    // 根据重要性范围查询事件记忆
    std::vector<EpisodicMemory> getByImportanceRange(float minImportance, float maxImportance) {
        return find(sqlite_orm::where(sqlite_orm::c(&EpisodicMemory::importance) >= minImportance and
                                     sqlite_orm::c(&EpisodicMemory::importance) <= maxImportance));
    }
};