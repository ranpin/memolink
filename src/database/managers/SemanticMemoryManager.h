#ifndef SEMANTIC_MEMORY_MANAGER_H
#define SEMANTIC_MEMORY_MANAGER_H

#include <string>
#include <vector>

struct SemanticMemoryRecord {
    int id;
    int user_id;
    int agent_id;
    std::string name;
    std::string summary;
    std::string details;
    std::string tree_path;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class SemanticMemoryManager {
public:
    static bool addSemanticMemory(int userId, int agentId, const std::string& name,
                                const std::string& summary, const std::string& details,
                                const std::string& treePath, const std::string& metadata,
                                const std::string& dbPath);
    
    static bool updateSemanticMemory(int memoryId, const std::string& name,
                                   const std::string& summary, const std::string& details,
                                   const std::string& metadata, const std::string& dbPath);
    
    static bool deleteSemanticMemory(int memoryId, const std::string& dbPath);
    
    // 添加查询功能
    static std::vector<SemanticMemoryRecord> getAllSemanticMemories(int userId, const std::string& dbPath);
    static SemanticMemoryRecord getSemanticMemoryById(int memoryId, const std::string& dbPath);
    static std::vector<SemanticMemoryRecord> searchSemanticMemories(int userId, const std::string& keyword, const std::string& dbPath);
};

#endif // SEMANTIC_MEMORY_MANAGER_H