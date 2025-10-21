#ifndef INIT_DB_H
#define INIT_DB_H

#include <string>

class InitDB {
public:
    static bool initializeDatabase(const std::string& dbPath);
    static bool createTables(const std::string& dbPath);
};

#endif // INIT_DB_H