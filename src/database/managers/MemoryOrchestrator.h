#ifndef MEMORY_ORCHESTRATOR_H
#define MEMORY_ORCHESTRATOR_H

#include <string>
#include <vector>
#include "../core/StorageManager.h"
#include "../../models/ModelManager.h"
#include "RecallMemoryManager.h"
#include "ArchivalMemoryManager.h"
#include "../../models/EmbeddingIndexManager.h"

class MemoryOrchestrator {
private:
    StorageManager& storage;
    ModelManager& model;
    RecallMemoryManager recallMemoryManager;
    ArchivalMemoryManager archivalMemoryManager;
    EmbeddingIndexManager embeddingIndexManager;

public:
    MemoryOrchestrator(StorageManager& storage, ModelManager& model);
    
    bool processMemory(const std::string& userId, const std::string& sessionId, 
                      const std::string& agentId, const std::string& memoryType,
                      const std::string& content, const std::string& metadata);
    
    std::string retrieveMemory(const std::string& userId, const std::string& query, 
                              const std::string& queryType, int topK = 10);
    
    bool add_memory_with_embedding(const std::string& memoryType,
                                  const std::string& content,
                                  const std::vector<float>& embedding,
                                  const std::string& metadata);
    
    std::vector<int> search_memory_by_embedding(const std::vector<float>& queryEmbedding,
                                               int topK);
};

#endif // MEMORY_ORCHESTRATOR_H