#ifndef EPISODIC_MEMORY_MANAGER_H
#define EPISODIC_MEMORY_MANAGER_H

#include <string>
#include <vector>

struct EpisodicMemoryRecord {
    int id;
    int user_id;
    int agent_id;
    std::string timestamp;
    std::string event;
    std::string details;
    std::string event_type;
    std::string tree_path;
    std::string metadata;
    std::string created_at;
    std::string updated_at;
};

class EpisodicMemoryManager {
public:
    static bool addEpisodicMemory(int userId, int agentId, const std::string& timestamp,
                                const std::string& event, const std::string& details,
                                const std::string& eventType, const std::string& treePath,
                                const std::string& metadata, const std::string& dbPath);
    
    static bool updateEpisodicMemory(int memoryId, const std::string& event,
                                   const std::string& details, const std::string& metadata,
                                   const std::string& dbPath);
    
    static bool deleteEpisodicMemory(int memoryId, const std::string& dbPath);
    
    // 添加查询功能
    static std::vector<EpisodicMemoryRecord> getAllEpisodicMemories(int userId, const std::string& dbPath);
    static EpisodicMemoryRecord getEpisodicMemoryById(int memoryId, const std::string& dbPath);
    static std::vector<EpisodicMemoryRecord> searchEpisodicMemories(int userId, const std::string& keyword, const std::string& dbPath);
};

#endif // EPISODIC_MEMORY_MANAGER_H