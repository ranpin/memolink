#include <vector>
#include <string>
#include "legacy/core-v1/SemanticMemoryManager.h"
#include "infra/storage/StorageManager.h"
#include <sqlite3.h>
#include <iostream>

bool SemanticMemoryManager::addSemanticMemory(int userId, int agentId, const std::string& name,
                                            const std::string& summary, const std::string& details,
                                            const std::string& treePath, const std::string& metadata,
                                            const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO semantic_memory (user_id, agent_id, name, title, content, description, path, metadata, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, agentId);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, summary.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, summary.c_str(), -1, SQLITE_STATIC); // 使用summary作为description
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

bool SemanticMemoryManager::updateSemanticMemory(int memoryId, const std::string& name,
                                               const std::string& summary, const std::string& details,
                                               const std::string& metadata, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "UPDATE semantic_memory SET name = ?, title = ?, content = ?, description = ?, metadata = ?, updated_at = datetime('now') WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, summary.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, summary.c_str(), -1, SQLITE_STATIC); // 使用summary作为description
    sqlite3_bind_text(stmt, 5, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, memoryId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool SemanticMemoryManager::deleteSemanticMemory(int memoryId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return false;
    }
    
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM semantic_memory WHERE id = ?;";
    
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

std::vector<SemanticMemoryRecord> SemanticMemoryManager::getAllSemanticMemories(int userId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return std::vector<SemanticMemoryRecord>();
    }
    
    std::vector<SemanticMemoryRecord> memories;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, name, title, content, description, path, metadata, created_at FROM semantic_memory WHERE user_id = ? ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return memories;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SemanticMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        memories.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return memories;
}

SemanticMemoryRecord SemanticMemoryManager::getSemanticMemoryById(int memoryId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return SemanticMemoryRecord();
    }
    
    SemanticMemoryRecord record;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, name, title, content, description, path, metadata, created_at FROM semantic_memory WHERE id = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return record;
    }
    
    sqlite3_bind_int(stmt, 1, memoryId);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

std::vector<SemanticMemoryRecord> SemanticMemoryManager::searchSemanticMemories(int userId, const std::string& keyword, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return std::vector<SemanticMemoryRecord>();
    }
    
    std::vector<SemanticMemoryRecord> memories;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, agent_id, name, title, content, description, path, metadata, created_at FROM semantic_memory WHERE user_id = ? AND (name LIKE ? OR title LIKE ? OR content LIKE ?) ORDER BY created_at DESC;";
    
    if (!storage.executeQuery(query, stmt)) {
        return memories;
    }
    
    std::string searchKeyword = "%" + keyword + "%";
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, searchKeyword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, searchKeyword.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SemanticMemoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.user_id = sqlite3_column_int(stmt, 1);
        record.agent_id = sqlite3_column_int(stmt, 2);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.summary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.tree_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        memories.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return memories;
}