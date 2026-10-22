#include <string>
// 已移至 src/database/core/StorageManager.cpp
#include "legacy/storage-stub/StorageManager.h"
#include <iostream>

StorageManager::StorageManager(const std::string& dbPath) : dbPath_(dbPath), db_(nullptr) {}

StorageManager::~StorageManager() {
    close();
}

bool StorageManager::open() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    return true;
}

void StorageManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool StorageManager::execute(const std::string& sql) {
    char* errMsg = 0;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}