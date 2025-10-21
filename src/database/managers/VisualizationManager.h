#ifndef VISUALIZATION_MANAGER_H
#define VISUALIZATION_MANAGER_H

#include <string>
#include <vector>
#include "../core/StorageManager.h"
#include "RecallMemoryManager.h"

struct MemoryNode {
    int id;
    std::string type;
    std::string content;
    std::string timestamp;
};

struct MemoryEdge {
    int sourceId;
    int targetId;
    std::string relationship;
    float weight;
};

class VisualizationManager {
private:
    StorageManager& storage;
    RecallMemoryManager recallMemoryManager;

public:
    VisualizationManager(StorageManager& storage);
    
    std::vector<MemoryNode> getMemoryNodes(int userId, int limit = 100);
    std::vector<MemoryEdge> getMemoryEdges(int userId, int limit = 100);
    std::string generateGraphJSON(int userId, int limit = 100);
    std::string generateTimelineData(int userId, int limit = 100);
};

#endif // VISUALIZATION_MANAGER_H