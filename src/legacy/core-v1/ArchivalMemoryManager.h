#ifndef ARCHIVAL_MEMORY_MANAGER_H
#define ARCHIVAL_MEMORY_MANAGER_H

#include <string>
#include <vector>

struct ArchivalMemoryRecord {
    int id;
    int user_id;
    int agent_id;
    std::string memory_type;
    std::string content;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class ArchivalMemoryManager {
public:
    static bool addArchivalMemory(int userId, int agentId, const std::string& memoryType,
                                const std::string& content, const std::string& metadata, 
                                const std::string& dbPath);
    
    static bool updateArchivalMemory(int memoryId, const std::string& content,
                                   const std::string& metadata, const std::string& dbPath);
    
    static bool deleteArchivalMemory(int memoryId, const std::string& dbPath);
    
    // 添加查询功能
    static std::vector<ArchivalMemoryRecord> getAllArchivalMemories(int userId, const std::string& dbPath);
    static ArchivalMemoryRecord getArchivalMemoryById(int memoryId, const std::string& dbPath);
    static std::vector<ArchivalMemoryRecord> searchArchivalMemories(int userId, const std::string& keyword, const std::string& dbPath);
};

#endif // ARCHIVAL_MEMORY_MANAGER_H