#pragma once
#include <string>
#include <sqlite3.h>

class StorageManager {
public:
    StorageManager(const std::string& dbPath);
    ~StorageManager();
    
    bool open();
    void close();
    bool execute(const std::string& sql);
    sqlite3* db() const { return db_; }
    
private:
    std::string dbPath_;
    sqlite3* db_;
};
// 已移至 src/database/core/StorageManager.h
