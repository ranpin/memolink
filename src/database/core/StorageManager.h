#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <sqlite3.h>
#include <memory>
#include "ORMWrapper.h"

class StorageManager {
private:
    std::string dbPath;
    sqlite3* db;
    std::unique_ptr<ORMWrapper> ormWrapper;
    
public:
    StorageManager(const std::string& path);
    ~StorageManager();
    
    bool initialize();
    
    const std::string& getDatabasePath() const { return dbPath; }
    sqlite3* getDatabase() const { return db; }
    
    // 提供对ORMWrapper的访问
    ORMWrapper& getORMWrapper() { return *ormWrapper; }
};

#endif // STORAGE_MANAGER_H