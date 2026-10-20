#include <utility>
#include <vector>
#include <string>
#include "CoreMemoryBlockManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

CoreMemoryBlockManager::CoreMemoryBlockManager(StorageManager& storage) : storage(storage) {}

bool CoreMemoryBlockManager::addCoreMemoryBlock(int userId, const std::string& label, 
                                               const std::string& value, const std::string& encryptedValue) {
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO core_memory_block (user_id, label, value, encrypted_value) VALUES (?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, label.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, encryptedValue.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<std::pair<std::string, std::string>> CoreMemoryBlockManager::listCoreMemoryBlocks(int userId) {
    std::vector<std::pair<std::string, std::string>> blocks;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT label, value FROM core_memory_block WHERE user_id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return blocks;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        blocks.push_back(std::make_pair(label, value));
    }
    
    sqlite3_finalize(stmt);
    
    return blocks;
}

bool CoreMemoryBlockManager::updateCoreMemoryBlock(int userId, const std::string& label, 
                                                  const std::string& value, const std::string& encryptedValue) {
    sqlite3_stmt* stmt;
    std::string query = "UPDATE core_memory_block SET value = ?, encrypted_value = ?, updated_at = CURRENT_TIMESTAMP WHERE user_id = ? AND label = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encryptedValue.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, userId);
    sqlite3_bind_text(stmt, 4, label.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool CoreMemoryBlockManager::deleteCoreMemoryBlock(int userId, const std::string& label) {
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM core_memory_block WHERE user_id = ? AND label = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, label.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<CoreMemoryBlockRecord> CoreMemoryBlockManager::getAllCoreMemoryBlocks(int userId) {
    std::vector<CoreMemoryBlockRecord> blocks;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, label, value, encrypted_value, created_at, updated_at FROM core_memory_block WHERE user_id = ? ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return blocks;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CoreMemoryBlockRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        const char* encryptedValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.encrypted_value = encryptedValue ? encryptedValue : "";
        
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.last_updated = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        
        blocks.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    
    return blocks;
}

CoreMemoryBlockRecord CoreMemoryBlockManager::getCoreMemoryBlockById(int id) {
    CoreMemoryBlockRecord record;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, label, value, encrypted_value, created_at, updated_at FROM core_memory_block WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        const char* encryptedValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.encrypted_value = encryptedValue ? encryptedValue : "";
        
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.last_updated = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    
    sqlite3_finalize(stmt);
    
    return record;
}

std::vector<CoreMemoryBlockRecord> CoreMemoryBlockManager::searchCoreMemoryBlocks(int userId, const std::string& keyword) {
    std::vector<CoreMemoryBlockRecord> blocks;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, label, value, encrypted_value, created_at, updated_at FROM core_memory_block WHERE user_id = ? AND (label LIKE ? OR value LIKE ?) ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return blocks;
    }
    
    std::string searchKeyword = "%" + keyword + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchKeyword.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CoreMemoryBlockRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        
        const char* encryptedValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.encrypted_value = encryptedValue ? encryptedValue : "";
        
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.last_updated = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        
        blocks.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    
    return blocks;
}