#pragma once

// KnowledgeVaultAgent - 知识库代理，管理用户的长期知识存储

#include "../database/core/StorageManager.h"
#include "../database/managers/KnowledgeVaultManager.h"
#include <string>
#include <vector>

struct KnowledgeVaultItem {
    int id;
    int user_id;
    std::string title;
    std::string content;
    std::string category;
    std::string tags;
    std::string source;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class KnowledgeVaultAgent {
private:
    StorageManager& storage_;
    KnowledgeVaultManager knowledge_vault_manager_;

public:
    // 构造函数
    KnowledgeVaultAgent(StorageManager& storage);
    
    // 知识库操作
    int add_knowledge_entry(int user_id, const std::string& title,
                           const std::string& content, const std::string& category = "",
                           const std::string& tags = "", const std::string& source = "",
                           const std::string& metadata = "");
    bool update_knowledge_entry(int entry_id, const std::string& title,
                               const std::string& content, const std::string& category = "",
                               const std::string& tags = "", const std::string& source = "",
                               const std::string& metadata = "");
    bool delete_knowledge_entry(int entry_id);
    std::vector<KnowledgeVaultItem> list_knowledge_entries(int user_id, int limit = 50);
    
    // 知识库检索
    std::vector<KnowledgeVaultItem> search_knowledge_entries(int user_id, const std::string& query);
    
    // 知识库摘要
    std::string get_knowledge_vault_summary(int user_id, int limit = 10);
};