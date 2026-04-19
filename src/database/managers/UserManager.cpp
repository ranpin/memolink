#include "UserManager.h"
#include "../core/StorageManager.h"
#include "../core/ORMWrapper.h"
#include <iostream>
#include <sqlite3.h>
#include <sstream>

std::string UserManager::hashPassword(const std::string& password) {
    return "hashed_" + password;
}

bool UserManager::addUser(const std::string& name, const std::string& email, 
                         const std::string& passwordHash, const std::string& dbPath) {
    try {
        ORMWrapper ormWrapper(dbPath);
        
        User user;
        user.name = name;
        user.email = email;
        user.password_hash = passwordHash;
        
        int userId = ormWrapper.insertUser(user);
        return userId > 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to add user: " << e.what() << std::endl;
        return false;
    }
}

int UserManager::getUserIdByName(const std::string& name, const std::string& dbPath) {
    try {
        ORMWrapper ormWrapper(dbPath);
        
        auto user = ormWrapper.getUserByName(name);
        if (user.has_value()) {
            return user.value().id;
        }
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get user by name: " << e.what() << std::endl;
        return -1;
    }
}

