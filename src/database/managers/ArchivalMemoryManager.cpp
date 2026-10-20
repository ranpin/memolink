#include <vector>
#include <string>
#include "ArchivalMemoryManager.h"
#include "../core/StorageManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

bool ArchivalMemoryManager::addArchivalMemory(int userId, int agentId, const std::string& memoryType,
                                             const std::string& content, const std::string& metadata, 
                                             const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO archival_memory (user_id, agent_id, memory_type, content, metadata) VALUES (?, ?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, agentId);
    sqlite3_bind_text(stmt, 3, memoryType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, metadata.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool ArchivalMemoryManager::updateArchivalMemory(int memoryId, const std::string& content,
                                               const std::string& metadata, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "UPDATE archival_memory SET content = ?, metadata = ? WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, memoryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool ArchivalMemoryManager::deleteArchivalMemory(int memoryId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM archival_memory WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<ArchivalMemoryRecord> ArchivalMemoryManager::getAllArchivalMemories(int userId, const std::string& dbPath) {
    std::vector<ArchivalMemoryRecord> records;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return records;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, memory_type, content, metadata, created_at, updated_at FROM archival_memory WHERE user_id = ? ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ArchivalMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}

ArchivalMemoryRecord ArchivalMemoryManager::getArchivalMemoryById(int memoryId, const std::string& dbPath) {
    ArchivalMemoryRecord record;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return record;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, memory_type, content, metadata, created_at, updated_at FROM archival_memory WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

std::vector<ArchivalMemoryRecord> ArchivalMemoryManager::searchArchivalMemories(int userId, const std::string& keyword, const std::string& dbPath) {
    std::vector<ArchivalMemoryRecord> records;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return records;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, memory_type, content, metadata, created_at, updated_at FROM archival_memory WHERE user_id = ? AND (content LIKE ? OR metadata LIKE ? OR memory_type LIKE ?) ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    std::string searchKeyword = "%" + keyword + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, searchKeyword.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ArchivalMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.memory_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}