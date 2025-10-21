#ifndef AUDIT_LOG_MANAGER_H
#define AUDIT_LOG_MANAGER_H

#include <string>

class AuditLogManager {
public:
    static bool addAuditLog(int userId, int agentId, const std::string& actionType,
                           int memoryId, const std::string& memoryType, const std::string& metadata,
                           int sessionId, const std::string& dbPath);
};

#endif // AUDIT_LOG_MANAGER_H