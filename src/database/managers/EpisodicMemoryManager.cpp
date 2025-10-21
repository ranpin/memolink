#include "EpisodicMemoryManager.h"
#include "../core/StorageManager.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

bool EpisodicMemoryManager::addEpisodicMemory(int userId, int agentId, const std::string& timestamp,
                                             const std::string& event, const std::string& details,
                                             const std::string& eventType, const std::string& treePath,
                                             const std::string& metadata, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO episodic_memory (user_id, agent_id, timestamp, event, details, event_type, tree_path, metadata) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, agentId);
    sqlite3_bind_text(stmt, 3, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, event.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, eventType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, treePath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, metadata.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool EpisodicMemoryManager::updateEpisodicMemory(int memoryId, const std::string& event,
                                               const std::string& details, const std::string& metadata,
                                               const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "UPDATE episodic_memory SET event = ?, details = ?, metadata = ? WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, event.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, memoryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool EpisodicMemoryManager::deleteEpisodicMemory(int memoryId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM episodic_memory WHERE id = ?;";
    
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

std::vector<EpisodicMemoryRecord> EpisodicMemoryManager::getAllEpisodicMemories(int userId, const std::string& dbPath) {
    std::vector<EpisodicMemoryRecord> records;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return records;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, timestamp, event, details, event_type, tree_path, metadata, created_at, updated_at FROM episodic_memory WHERE user_id = ? ORDER BY timestamp DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EpisodicMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}

EpisodicMemoryRecord EpisodicMemoryManager::getEpisodicMemoryById(int memoryId, const std::string& dbPath) {
    EpisodicMemoryRecord record;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return record;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, timestamp, event, details, event_type, tree_path, metadata, created_at, updated_at FROM episodic_memory WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

std::vector<EpisodicMemoryRecord> EpisodicMemoryManager::searchEpisodicMemories(int userId, const std::string& keyword, const std::string& dbPath) {
    std::vector<EpisodicMemoryRecord> records;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return records;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, timestamp, event, details, event_type, tree_path, metadata, created_at, updated_at FROM episodic_memory WHERE user_id = ? AND (event LIKE ? OR details LIKE ? OR metadata LIKE ?) ORDER BY timestamp DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return records;
    }
    
    std::string searchKeyword = "%" + keyword + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, searchKeyword.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EpisodicMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.event = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        records.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return records;
}