#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include "StorageManager.h"
#include <string>

class TransactionManager {
private:
    StorageManager& storage_;

public:
    explicit TransactionManager(StorageManager& storage);
    
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // RAII风格的事务管理
    class TransactionGuard {
    private:
        TransactionManager& manager_;
        bool committed_;
        
    public:
        explicit TransactionGuard(TransactionManager& manager);
        ~TransactionGuard();
        void commit();
    };
};

#endif // TRANSACTION_MANAGER_H