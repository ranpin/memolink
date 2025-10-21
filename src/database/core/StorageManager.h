#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <sqlite3.h>

class StorageManager {
private:
    std::string dbPath;
    sqlite3* db;
    
    bool enableForeignKeys();

public:
    StorageManager(const std::string& path);
    ~StorageManager();
    
    bool initialize();
    bool executeQuery(const std::string& query);
    bool executeQuery(const std::string& query, sqlite3_stmt*& stmt);
    bool executeNonQuery(const std::string& query); // 添加这一行
    
    sqlite3* getDatabase() { return db; }
    std::string getDatabasePath() { return dbPath; }
};

#endif // STORAGE_MANAGER_H