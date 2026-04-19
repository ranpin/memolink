#include "MemoryOrchestratorImpl.h"
#include "MessageMemoryManager.h"
#include "SemanticMemoryManager.h"
#include "EpisodicMemoryManager.h"
#include "ProceduralMemoryManager.h"
#include "CoreMemoryBlockManager.h"
#include "SecretVaultManager.h"
#include "UserManager.h"
#include "SessionManager.h"

MemoryOrchestratorImpl::MemoryOrchestratorImpl(ORMWrapper& orm) 
    : MemoryOrchestrator(orm), orm_(orm) {
    // 在这里不需要做任何事情，因为我们将在需要时延迟初始化各个Manager
}

MessageMemoryManager& MemoryOrchestratorImpl::getMessageMemoryManager() {
    if (!messageMemoryManager_) {
        messageMemoryManager_ = std::make_unique<MessageMemoryManager>(orm_);
    }
    return *messageMemoryManager_;
}

SemanticMemoryManager& MemoryOrchestratorImpl::getSemanticMemoryManager() {
    if (!semanticMemoryManager_) {
        semanticMemoryManager_ = std::make_unique<SemanticMemoryManager>(orm_);
    }
    return *semanticMemoryManager_;
}

EpisodicMemoryManager& MemoryOrchestratorImpl::getEpisodicMemoryManager() {
    if (!episodicMemoryManager_) {
        episodicMemoryManager_ = std::make_unique<EpisodicMemoryManager>(orm_);
    }
    return *episodicMemoryManager_;
}

ProceduralMemoryManager& MemoryOrchestratorImpl::getProceduralMemoryManager() {
    if (!proceduralMemoryManager_) {
        proceduralMemoryManager_ = std::make_unique<ProceduralMemoryManager>(orm_);
    }
    return *proceduralMemoryManager_;
}

CoreMemoryBlockManager& MemoryOrchestratorImpl::getCoreMemoryBlockManager() {
    if (!coreMemoryBlockManager_) {
        coreMemoryBlockManager_ = std::make_unique<CoreMemoryBlockManager>(orm_);
    }
    return *coreMemoryBlockManager_;
}



SecretVaultManager& MemoryOrchestratorImpl::getSecretVaultManager() {
    if (!secretVaultManager_) {
        secretVaultManager_ = std::make_unique<SecretVaultManager>(orm_);
    }
    return *secretVaultManager_;
}

UserManager& MemoryOrchestratorImpl::getUserManager() {
    if (!userManager_) {
        // UserManager是一个静态类，不需要实例化
        static UserManager instance;
        userManager_ = std::make_unique<UserManager>(instance); // 使用拷贝构造创建unique_ptr
    }
    return *userManager_;
}

SessionManager& MemoryOrchestratorImpl::getSessionManager() {
    if (!sessionManager_) {
        // 由于SessionManager需要StorageManager而我们只有ORMWrapper，
        // 我们暂时创建一个指向ORMWrapper的StorageManager
        // 注意：这可能不是一个长期可行的解决方案
        static StorageManager storage(""); // 使用默认构造函数
        sessionManager_ = std::make_unique<SessionManager>(storage);
    }
    return *sessionManager_;
}