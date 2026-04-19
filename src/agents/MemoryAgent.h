#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../database/core/StorageManager.h"
#include "../database/managers/MemoryOrchestrator.h"
#include "../database/core/ORMWrapper.h"

class MemoryAgent {
private:
    StorageManager& storage_;
    std::unique_ptr<MemoryOrchestrator> orchestrator_;

public:
    explicit MemoryAgent(StorageManager& storage);
    ~MemoryAgent() = default;

    // 核心记忆操作
    bool addCoreMemoryBlock(int user_id, const std::string& key, const std::string& value, const std::string& metadata);
    std::map<std::string, std::string> getCoreMemoryBlock(int user_id);
    bool updateCoreMemoryBlock(int user_id, const std::string& key, const std::string& value, const std::string& metadata);
    bool deleteCoreMemoryBlock(int user_id, const std::string& key);

    // 语义记忆操作
    std::vector<std::map<std::string, std::string>> listSemanticMemories(int user_id, int limit = 10);
};
