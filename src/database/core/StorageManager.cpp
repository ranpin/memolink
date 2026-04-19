#include "StorageManager.h"
#include <iostream>
#include <sqlite3.h>

StorageManager::StorageManager(const std::string& path) 
    : dbPath(path), db(nullptr), ormWrapper(nullptr) {}

StorageManager::~StorageManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool StorageManager::initialize() {
    // 设置SQLite线程安全模式
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // 设置数据库连接选项以支持并发访问
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA locking_mode=NORMAL;", NULL, NULL, NULL);
    
    // 初始化ORMWrapper并传递数据库连接
    ormWrapper = std::make_unique<ORMWrapper>(dbPath, db);
    
    // 初始化 embedding_index 表
    const char* createEmbeddingIndexSQL = R"(
        CREATE TABLE IF NOT EXISTS embedding_index (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            memory_id INTEGER NOT NULL,
            memory_type TEXT NOT NULL,
            embedding BLOB NOT NULL,
            metadata TEXT,
            created_at TEXT NOT NULL
        )
    )";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createEmbeddingIndexSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create embedding_index table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    std::cout << "StorageManager initialized successfully" << std::endl;
    return true;
}