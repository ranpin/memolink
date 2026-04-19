#ifndef SECRET_VAULT_GENERATOR_H
#define SECRET_VAULT_GENERATOR_H

#include "BaseMemoryGenerator.h"
#include <string>
#include <vector>
#include "../../messages/MessageManager.h"
#include "../../../database/managers/MemoryOrchestrator.h"

class SecretVaultGenerator : public BaseMemoryGenerator {
public:
    struct GeneratedSecretVault {
        std::string secret_type;
        std::string title;
        std::string content;
        std::string access_level;
        std::vector<std::string> tags;
        std::vector<std::string> related_secrets;
        std::string expiration_date;
        float importance = 0.0f;
    };

private:
    // 实现基类的纯虚函数
    std::string buildGenerationPrompt(const std::vector<MessageManager::Message>& messages) override;
    bool saveToDatabaseImpl(const std::string& jsonResult,
                           int user_id, int agent_id, int session_id,
                           MemoryOrchestrator& storage) override;
                           
    GeneratedSecretVault parseGeneratedMemory(const std::string& jsonResult);
    
public:
    using BaseMemoryGenerator::generate;
    using BaseMemoryGenerator::processAndSave;
};

#endif // SECRET_VAULT_GENERATOR_H