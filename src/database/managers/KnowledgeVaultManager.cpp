#include "KnowledgeVaultManager.h"
#include "../core/TransactionManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

KnowledgeVaultManager::KnowledgeVaultManager(StorageManager& storage) : storage(storage) {
}

bool KnowledgeVaultManager::addKnowledgeVaultEntry(int userId, int agentId, const std::string& caption,
                                                 const std::string& entryType, const std::string& source,
                                                 const std::string& sensitivity, const std::string& encryptedValue,
                                                 const std::string& metadata) {
    TransactionManager transactionManager(storage);
    TransactionManager::TransactionGuard guard(transactionManager);
    
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO knowledge_vault (user_id, agent_id, caption, entry_type, source, sensitivity, encrypted_value, metadata) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, agentId);
    sqlite3_bind_text(stmt, 3, caption.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entryType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, source.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, sensitivity.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, encryptedValue.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, metadata.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    // 如果所有操作都成功，则提交事务
    guard.commit();
    return true;
}

bool KnowledgeVaultManager::updateKnowledgeVaultEntry(int entryId, const std::string& caption,
                                                    const std::string& entryType, const std::string& source,
                                                    const std::string& sensitivity, const std::string& encryptedValue,
                                                    const std::string& metadata) {
    TransactionManager transactionManager(storage);
    TransactionManager::TransactionGuard guard(transactionManager);
    
    sqlite3_stmt* stmt;
    std::string query = "UPDATE knowledge_vault SET caption = ?, entry_type = ?, source = ?, sensitivity = ?, encrypted_value = ?, metadata = ? WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, caption.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entryType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, source.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, sensitivity.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, encryptedValue.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, entryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    // 如果所有操作都成功，则提交事务
    guard.commit();
    return true;
}

bool KnowledgeVaultManager::deleteKnowledgeVaultEntry(int entryId) {
    TransactionManager transactionManager(storage);
    TransactionManager::TransactionGuard guard(transactionManager);
    
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM knowledge_vault WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, entryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    // 如果所有操作都成功，则提交事务
    guard.commit();
    return true;
}

std::vector<KnowledgeVaultRecord> KnowledgeVaultManager::getAllKnowledgeVaultEntries(int userId) {
    std::vector<KnowledgeVaultRecord> records;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, caption, entry_type, source, sensitivity, encrypted_value, metadata, created_at, updated_at FROM knowledge_vault WHERE user_id = ? ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        KnowledgeVaultRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.caption = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.entry_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.sensitivity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.encrypted_value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}

KnowledgeVaultRecord KnowledgeVaultManager::getKnowledgeVaultEntryById(int entryId) {
    KnowledgeVaultRecord record;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, caption, entry_type, source, sensitivity, encrypted_value, metadata, created_at, updated_at FROM knowledge_vault WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, entryId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.caption = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.entry_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.sensitivity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.encrypted_value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

std::vector<KnowledgeVaultRecord> KnowledgeVaultManager::searchKnowledgeVaultEntries(int userId, const std::string& keyword) {
    std::vector<KnowledgeVaultRecord> records;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, caption, entry_type, source, sensitivity, encrypted_value, metadata, created_at, updated_at FROM knowledge_vault WHERE user_id = ? AND (caption LIKE ? OR entry_type LIKE ? OR metadata LIKE ?) ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    std::string searchKeyword = "%" + keyword + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, searchKeyword.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        KnowledgeVaultRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.caption = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.entry_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.sensitivity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.encrypted_value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}
