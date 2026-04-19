#include "TransactionManager.h"
#include <iostream>

TransactionManager::TransactionManager(StorageManager& storage) : storage_(storage) {
}

bool TransactionManager::beginTransaction() {
    // 不再需要手动执行BEGIN TRANSACTION语句
    // ORMWrapper的transaction方法会自动处理事务开始
    return true;
}

bool TransactionManager::commitTransaction() {
    // 不再需要手动执行COMMIT语句
    // ORMWrapper的transaction方法会自动处理提交
    return true;
}

bool TransactionManager::rollbackTransaction() {
    // 不再需要手动执行ROLLBACK语句
    // ORMWrapper的transaction方法会自动处理回滚
    return true;
}

// TransactionGuard实现
TransactionManager::TransactionGuard::TransactionGuard(TransactionManager& manager) 
    : manager_(manager), committed_(false) {
    // ORMWrapper的transaction方法会自动处理事务开始
}

TransactionManager::TransactionGuard::~TransactionGuard() {
    // ORMWrapper的transaction方法会自动处理事务结束
}

void TransactionManager::TransactionGuard::commit() {
    if (!committed_) {
        // ORMWrapper的transaction方法会自动处理提交
        committed_ = true;
    }
}