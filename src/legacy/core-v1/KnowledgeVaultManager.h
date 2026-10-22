#ifndef KNOWLEDGE_VAULT_MANAGER_H
#define KNOWLEDGE_VAULT_MANAGER_H

#include "infra/storage/StorageManager.h"
#include <string>
#include <vector>

struct KnowledgeVaultRecord {
    int id;
    int user_id;
    int agent_id;
    std::string caption;
    std::string entry_type;
    std::string source;
    std::string sensitivity;
    std::string encrypted_value;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class KnowledgeVaultManager {
private:
    StorageManager& storage;

public:
    KnowledgeVaultManager(StorageManager& storage);
    
    bool addKnowledgeVaultEntry(int userId, int agentId, const std::string& caption,
                              const std::string& entryType, const std::string& source,
                              const std::string& sensitivity, const std::string& encryptedValue,
                              const std::string& metadata);
    
    bool updateKnowledgeVaultEntry(int entryId, const std::string& caption,
                                 const std::string& entryType, const std::string& source,
                                 const std::string& sensitivity, const std::string& encryptedValue,
                                 const std::string& metadata);
    
    bool deleteKnowledgeVaultEntry(int entryId);
    
    // 添加查询功能
    std::vector<KnowledgeVaultRecord> getAllKnowledgeVaultEntries(int userId);
    KnowledgeVaultRecord getKnowledgeVaultEntryById(int entryId);
    std::vector<KnowledgeVaultRecord> searchKnowledgeVaultEntries(int userId, const std::string& keyword);
};

#endif // KNOWLEDGE_VAULT_MANAGER_H