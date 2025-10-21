#include "StorageManager.h"
#include <iostream>
#include <sqlite3.h>

StorageManager::StorageManager(const std::string& path) : dbPath(path), db(nullptr) {
}

StorageManager::~StorageManager() {
    if (db) {
        sqlite3_close(db);
        db = nullptr; // 防止悬空指针
    }
}

bool StorageManager::initialize() {
    // 如果已经初始化，先关闭现有连接
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
    
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr; // 确保db指针被设置为nullptr
        return false;
    }
    
    // 启用外键约束
    enableForeignKeys();
    
    return true;
}

bool StorageManager::enableForeignKeys() {
    char* errMsg = 0;
    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to enable foreign keys: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool StorageManager::executeQuery(const std::string& query) {
    // 确保数据库连接有效
    if (!db) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }
    
    // 确保外键约束已启用
    enableForeignKeys();
    
    char* errMsg = 0;
    int rc = sqlite3_exec(db, query.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool StorageManager::executeQuery(const std::string& query, sqlite3_stmt*& stmt) {
    // 确保数据库连接有效
    if (!db) {
        std::cerr << "Database not initialized" << std::endl;
        return false;
    }
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

// 添加executeNonQuery函数实现
bool StorageManager::executeNonQuery(const std::string& query) {
    sqlite3_stmt* stmt;
    if (!executeQuery(query, stmt)) {
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    return true;
}