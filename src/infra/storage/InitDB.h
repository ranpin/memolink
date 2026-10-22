#ifndef INIT_DB_H
#define INIT_DB_H

#include <string>

class InitDB {
public:
    // 创建 v2 端侧记忆 schema（仅 memory 表 + 索引）。
    // 原 createTables（v1 的 12 张表 + 触发器）已随 legacy 移出构建。
    static bool initializeDatabase(const std::string& dbPath);
};

#endif // INIT_DB_H