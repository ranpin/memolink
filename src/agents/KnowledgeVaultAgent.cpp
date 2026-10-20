#include <vector>
#include <string>
#include "KnowledgeVaultAgent.h"

KnowledgeVaultAgent::KnowledgeVaultAgent(StorageManager& storage) : 
    storage_(storage),
    knowledge_vault_manager_(storage) {
}

int KnowledgeVaultAgent::add_knowledge_entry(int user_id, const std::string& title,
                                             const std::string& content, const std::string& category,
                                             const std::string& tags, const std::string& source,
                                             const std::string& metadata) {
    // 创建一个知识条目
    KnowledgeVaultItem item;
    item.id = 0; // 数据库将生成ID
    item.user_id = user_id;
    item.title = title;
    item.content = content;
    item.category = category.empty() ? "general" : category;
    item.tags = tags;
    item.source = source.empty() ? "user_input" : source;
    item.metadata = metadata.empty() ? "{}" : metadata;
    
    // 将条目保存到数据库
    bool result = knowledge_vault_manager_.addKnowledgeVaultEntry(
        item.user_id,
        0, // agentId暂时设为0
        item.title,
        item.category,
        item.source,
        "normal",
        item.content,
        item.metadata
    );
    
    // 返回新创建的条目ID（简化处理）
    return result ? 1 : -1;
}

bool KnowledgeVaultAgent::update_knowledge_entry(int entry_id, const std::string& title,
                                                 const std::string& content, const std::string& category,
                                                 const std::string& tags, const std::string& source,
                                                 const std::string& metadata) {
    // 更新知识条目
    return knowledge_vault_manager_.updateKnowledgeVaultEntry(
        entry_id,
        title,
        category,
        source,
        "normal",
        content,
        metadata
    );
}

bool KnowledgeVaultAgent::delete_knowledge_entry(int entry_id) {
    // 删除知识条目
    return knowledge_vault_manager_.deleteKnowledgeVaultEntry(entry_id);
}

std::vector<KnowledgeVaultItem> KnowledgeVaultAgent::list_knowledge_entries(int user_id, int limit) {
    // 从数据库获取所有知识条目
    std::vector<KnowledgeVaultRecord> records = knowledge_vault_manager_.getAllKnowledgeVaultEntries(user_id);
    
    // 转换为KnowledgeVaultItem向量
    std::vector<KnowledgeVaultItem> items;
    items.reserve(records.size());
    
    for (const auto& record : records) {
        KnowledgeVaultItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.title = record.caption;
        item.content = record.encrypted_value;
        item.category = record.entry_type;
        item.source = record.source;
        item.metadata = record.metadata;
        // 设置默认值因为KnowledgeVaultRecord没有对应字段
        item.tags = "";
        item.created_at = "";
        item.updated_at = "";
        items.push_back(item);
        
        // 检查限制
        if (static_cast<int>(items.size()) >= limit && limit > 0) {
            break;
        }
    }
    
    return items;
}

std::vector<KnowledgeVaultItem> KnowledgeVaultAgent::search_knowledge_entries(int user_id, const std::string& query) {
    // 搜索知识条目
    std::vector<KnowledgeVaultRecord> records = knowledge_vault_manager_.searchKnowledgeVaultEntries(user_id, query);
    
    // 转换为KnowledgeVaultItem向量
    std::vector<KnowledgeVaultItem> items;
    items.reserve(records.size());
    
    for (const auto& record : records) {
        KnowledgeVaultItem item;
        item.id = record.id;
        item.user_id = record.user_id;
        item.title = record.caption;
        item.content = record.encrypted_value;
        item.category = record.entry_type;
        item.source = record.source;
        item.metadata = record.metadata;
        // 设置默认值因为KnowledgeVaultRecord没有对应字段
        item.tags = "";
        item.created_at = "";
        item.updated_at = "";
        items.push_back(item);
    }
    
    return items;
}

std::string KnowledgeVaultAgent::get_knowledge_vault_summary(int user_id, int limit) {
    auto entries = list_knowledge_entries(user_id, limit);
    std::string summary = "Knowledge vault summary for user " + std::to_string(user_id) + ":\n";
    
    for (const auto& entry : entries) {
        summary += "- " + entry.title + " (" + entry.category + ")\n";
    }
    
    return summary;
}