#include <string>
#include "UserManager.h"
#include "../core/StorageManager.h"
#include <iostream>
#include <sqlite3.h>
#include <sstream>

std::string UserManager::hashPassword(const std::string& password) {
    return "hashed_" + password;
}

bool UserManager::addUser(const std::string& name, const std::string& email, 
                         const std::string& passwordHash, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO user (name, email) VALUES (?, ?);";
    
    if (!storage.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

int UserManager::getUserIdByName(const std::string& name, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return -1;
    }

    sqlite3_stmt* stmt;
    std::string query = "SELECT id FROM user WHERE name = ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return userId;
    } else {
        sqlite3_finalize(stmt);
        return -1;
    }
}

