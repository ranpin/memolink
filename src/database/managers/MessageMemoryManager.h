#pragma once

#include <vector>
#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"

// 回忆记忆管理器类
class MessageMemoryManager : public MemoryManager<MessageMemory> {
public:
    explicit MessageMemoryManager(ORMWrapper& orm) 
        : MemoryManager<MessageMemory>(orm) {}
        
    // 根据会话ID查询回忆记忆
    std::vector<MessageMemory> getBySessionId(int64_t sessionId) {
        return find(sqlite_orm::where(sqlite_orm::c(&MessageMemory::session_id) == sessionId));
    }
    
    // 根据用户ID和会话ID查询回忆记忆
    std::vector<MessageMemory> getByUserAndSession(int64_t userId, int64_t sessionId) {
        return find(sqlite_orm::where(sqlite_orm::c(&MessageMemory::user_id) == userId and 
                                     sqlite_orm::c(&MessageMemory::session_id) == sessionId));
    }
};