#pragma once

#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"

// 私密信息库管理器类
class SecretVaultManager : public MemoryManager<SecretVault> {
public:
    explicit SecretVaultManager(ORMWrapper& orm) 
        : MemoryManager<SecretVault>(orm) {}
        
    // 根据类别查询知识条目
    std::vector<SecretVault> getByCategory(const std::string& category) {
        return find(sqlite_orm::where(sqlite_orm::c(&SecretVault::category) == category));
    }
    
    // 根据标题查询知识条目
    std::vector<SecretVault> getByTitle(const std::string& title) {
        return find(sqlite_orm::where(sqlite_orm::c(&SecretVault::title) == title));
    }
};