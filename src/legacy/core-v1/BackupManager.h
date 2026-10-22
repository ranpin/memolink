#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>

class BackupManager {
public:
    static bool createBackup(const std::string& dbPath, const std::string& backupPath);
    static bool restoreBackup(const std::string& backupPath, const std::string& dbPath);
};

#endif // BACKUP_MANAGER_H