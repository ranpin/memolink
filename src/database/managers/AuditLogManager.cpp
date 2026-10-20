#include <string>
#include "AuditLogManager.h"
#include "../core/StorageManager.h"
#include <sqlite3.h>
#include <iostream>

bool AuditLogManager::addAuditLog(int userId, int agentId, const std::string& actionType,
                                 int memoryId, const std::string& memoryType, const std::string& metadata,
                                 int sessionId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO audit_log (user_id, agent_id, action_type, memory_id, memory_type, metadata, session_id) VALUES (?, ?, ?, ?, ?, ?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, agentId);
    sqlite3_bind_text(stmt, 3, actionType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, memoryId);
    sqlite3_bind_text(stmt, 5, memoryType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, sessionId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}