#ifndef MEMORY_ORCHESTRATOR_IMPL_H
#define MEMORY_ORCHESTRATOR_IMPL_H

#include "MemoryOrchestrator.h"
#include "MessageMemoryManager.h"
#include "SemanticMemoryManager.h"
#include "EpisodicMemoryManager.h"
#include "ProceduralMemoryManager.h"
#include "CoreMemoryBlockManager.h"
#include "SecretVaultManager.h"
#include "UserManager.h"
#include "SessionManager.h"
#include <memory>

// MemoryOrchestrator的具体实现类
class MemoryOrchestratorImpl : public MemoryOrchestrator {
public:
    explicit MemoryOrchestratorImpl(ORMWrapper& orm);
    
    // 实现所有纯虚函数
    MessageMemoryManager& getMessageMemoryManager() override;
    SemanticMemoryManager& getSemanticMemoryManager() override;
    EpisodicMemoryManager& getEpisodicMemoryManager() override;
    ProceduralMemoryManager& getProceduralMemoryManager() override;
    CoreMemoryBlockManager& getCoreMemoryBlockManager() override;
    SecretVaultManager& getSecretVaultManager() override;
    UserManager& getUserManager() override;
    SessionManager& getSessionManager() override;

private:
    // 按需延迟初始化各种MemoryManager
    std::unique_ptr<MessageMemoryManager> messageMemoryManager_;
    std::unique_ptr<SemanticMemoryManager> semanticMemoryManager_;
    std::unique_ptr<EpisodicMemoryManager> episodicMemoryManager_;
    std::unique_ptr<ProceduralMemoryManager> proceduralMemoryManager_;
    std::unique_ptr<CoreMemoryBlockManager> coreMemoryBlockManager_;
    std::unique_ptr<SecretVaultManager> secretVaultManager_;
    std::unique_ptr<UserManager> userManager_;
    std::unique_ptr<SessionManager> sessionManager_;
    
    // ORM包装器引用
    ORMWrapper& orm_;
};

#endif // MEMORY_ORCHESTRATOR_IMPL_H