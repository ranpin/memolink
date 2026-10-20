#include <string>
#include "BackupManager.h"
#include "../core/StorageManager.h"
#include <sqlite3.h>
#include <iostream>
#include <fstream>

bool BackupManager::createBackup(const std::string& dbPath, const std::string& backupPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    sqlite3* backupDb;
    int rc = sqlite3_open(backupPath.c_str(), &backupDb);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open backup database: " << sqlite3_errmsg(backupDb) << std::endl;
        sqlite3_close(backupDb);
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(backupDb, "main", storage.getDatabase(), "main");
    if (!backup) {
        std::cerr << "Failed to initialize backup: " << sqlite3_errmsg(backupDb) << std::endl;
        sqlite3_close(backupDb);
        return false;
    }

    do {
        rc = sqlite3_backup_step(backup, 100);
    } while (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);

    sqlite3_backup_finish(backup);
    sqlite3_close(backupDb);

    if (rc != SQLITE_DONE) {
        std::cerr << "Backup failed: " << sqlite3_errmsg(backupDb) << std::endl;
        return false;
    }

    return true;
}

bool BackupManager::restoreBackup(const std::string& backupPath, const std::string& dbPath) {
    // 检查备份文件是否存在
    std::ifstream backupFile(backupPath);
    if (!backupFile.good()) {
        std::cerr << "Backup file does not exist: " << backupPath << std::endl;
        return false;
    }
    backupFile.close();

    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }

    StorageManager backupStorage(backupPath);
    if (!backupStorage.initialize()) {
        std::cerr << "Failed to initialize backup database" << std::endl;
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(storage.getDatabase(), "main", backupStorage.getDatabase(), "main");
    if (!backup) {
        std::cerr << "Failed to initialize restore: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
        return false;
    }

    do {
        int rc = sqlite3_backup_step(backup, 100);
        if (rc != SQLITE_OK && rc != SQLITE_DONE) {
            std::cerr << "Restore step failed: " << sqlite3_errmsg(storage.getDatabase()) << std::endl;
            sqlite3_backup_finish(backup);
            return false;
        }
    } while (sqlite3_backup_remaining(backup) > 0);

    sqlite3_backup_finish(backup);
    return true;
}