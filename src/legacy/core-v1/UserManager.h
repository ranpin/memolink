#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>

class UserManager {
public:
    static bool addUser(const std::string& name, const std::string& email, const std::string& passwordHash, const std::string& dbPath);
    static bool updateUser(int userId, const std::string& name, const std::string& email, const std::string& passwordHash, const std::string& dbPath);
    static bool deleteUser(int userId, const std::string& dbPath);
    static int getUserIdByName(const std::string& name, const std::string& dbPath);
    
    // 安全增强功能
    static std::string hashPassword(const std::string& password);
    static bool verifyPassword(const std::string& password, const std::string& hash);
    static bool updatePassword(int userId, const std::string& newPassword, const std::string& dbPath);
};

#endif // USER_MANAGER_H