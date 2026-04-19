#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"
#include <string>

// 记忆溯源管理器，用于读写 MemoryProvenance 记录
class MemoryProvenanceManager : public MemoryManager<MemoryProvenance> {
public:
    explicit MemoryProvenanceManager(ORMWrapper& orm)
        : MemoryManager<MemoryProvenance>(orm) {}

    // 根据记忆类型和ID查询溯源记录
    std::vector<MemoryProvenance> getByMemory(const std::string& memory_type, int memory_id) {
        return this->find(sqlite_orm::where(
            sqlite_orm::c(&MemoryProvenance::memory_type) == memory_type &&
            sqlite_orm::c(&MemoryProvenance::memory_id) == memory_id
        ));
    }
};
