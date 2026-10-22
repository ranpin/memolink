#include <string>
#include "infra/storage/TransactionManager.h"
#include <iostream>

TransactionManager::TransactionManager(StorageManager& storage) : storage_(storage) {
}

bool TransactionManager::beginTransaction() {
    sqlite3_stmt* stmt;
    std::string query = "BEGIN TRANSACTION;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool TransactionManager::commitTransaction() {
    sqlite3_stmt* stmt;
    std::string query = "COMMIT;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to commit transaction: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool TransactionManager::rollbackTransaction() {
    sqlite3_stmt* stmt;
    std::string query = "ROLLBACK;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to rollback transaction: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

// TransactionGuard实现
TransactionManager::TransactionGuard::TransactionGuard(TransactionManager& manager) 
    : manager_(manager), committed_(false) {
    manager_.beginTransaction();
}

TransactionManager::TransactionGuard::~TransactionGuard() {
    if (!committed_) {
        manager_.rollbackTransaction();
    }
}

void TransactionManager::TransactionGuard::commit() {
    if (!committed_) {
        manager_.commitTransaction();
        committed_ = true;
    }
}