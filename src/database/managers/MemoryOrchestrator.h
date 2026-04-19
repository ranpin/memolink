#ifndef MEMORY_ORCHESTRATOR_H
#define MEMORY_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../core/ORMWrapper.h"
#include "../core/MemoryManager.h"
#include "../models/MemoryModels.h"

// 前向声明所有具体的MemoryManager类
class MessageMemoryManager;
class SemanticMemoryManager;
class EpisodicMemoryManager;
class ProceduralMemoryManager;
class CoreMemoryBlockManager;
class SecretVaultManager;
class UserManager;
class SessionManager;

class MemoryOrchestrator {
protected:
    ORMWrapper& ormWrapper_;

    // 使用工厂模式管理各种MemoryManager实例
    template<typename T>
    std::shared_ptr<MemoryManager<T>> getManager() {
        // 使用静态局部变量实现单例模式
        static std::shared_ptr<MemoryManager<T>> manager = 
            std::make_shared<MemoryManager<T>>(ormWrapper_);
        return manager;
    }

public:
    // 构造函数
    explicit MemoryOrchestrator(ORMWrapper& ormWrapper) : ormWrapper_(ormWrapper) {}
    virtual ~MemoryOrchestrator() = default;

    // 提供对ORMWrapper的访问
    ORMWrapper& getORMWrapper() { return ormWrapper_; }

    // 初始化方法
    virtual bool initialize() { 
        // 简单返回true，因为ORMWrapper会在首次访问时自动创建表
        return true; 
    }

    // 获取各个管理器的引用，需要子类实现
    virtual MessageMemoryManager& getMessageMemoryManager() = 0;
    virtual SemanticMemoryManager& getSemanticMemoryManager() = 0;
    virtual EpisodicMemoryManager& getEpisodicMemoryManager() = 0;
    virtual ProceduralMemoryManager& getProceduralMemoryManager() = 0;
    virtual CoreMemoryBlockManager& getCoreMemoryBlockManager() = 0;
    virtual SecretVaultManager& getSecretVaultManager() = 0;
    virtual UserManager& getUserManager() = 0;
    virtual SessionManager& getSessionManager() = 0;
    
    // 事务支持方法
    template<typename Func>
    void transaction(Func&& func) {
        ormWrapper_.transaction(std::forward<Func>(func));
    }
};

#endif // MEMORY_ORCHESTRATOR_H