#ifndef PROCEDURAL_MEMORY_MANAGER_H
#define PROCEDURAL_MEMORY_MANAGER_H

#include <string>
#include <vector>

struct ProceduralMemoryRecord {
    int id;
    int user_id;
    int agent_id;
    std::string title;
    std::string summary;
    std::string steps;
    std::string tree_path;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class ProceduralMemoryManager {
public:
    static bool addProceduralMemory(int userId, int agentId, const std::string& title,
                                  const std::string& summary, const std::string& steps,
                                  const std::string& treePath, const std::string& metadata,
                                  const std::string& dbPath);
    
    static bool updateProceduralMemory(int memoryId, const std::string& title,
                                     const std::string& summary, const std::string& steps,
                                     const std::string& metadata, const std::string& dbPath);
    
    static bool deleteProceduralMemory(int memoryId, const std::string& dbPath);
    
    // 添加查询功能
    static std::vector<ProceduralMemoryRecord> getAllProceduralMemories(int userId, const std::string& dbPath);
    static ProceduralMemoryRecord getProceduralMemoryById(int memoryId, const std::string& dbPath);
    static std::vector<ProceduralMemoryRecord> searchProceduralMemories(int userId, const std::string& keyword, const std::string& dbPath);
};

#endif // PROCEDURAL_MEMORY_MANAGER_H