#ifndef CORE_MEMORY_BLOCK_MANAGER_H
#define CORE_MEMORY_BLOCK_MANAGER_H
#include <utility>

#include <string>
#include <vector>
#include "infra/storage/StorageManager.h"

struct CoreMemoryBlockRecord {
    int id;
    int user_id;
    std::string label;
    std::string value;
    std::string encrypted_value;
    std::string created_at;
    std::string last_updated;
};

class CoreMemoryBlockManager {
private:
    StorageManager& storage;

public:
    CoreMemoryBlockManager(StorageManager& storage);
    
    bool addCoreMemoryBlock(int userId, const std::string& label, 
                           const std::string& value, const std::string& encryptedValue);
    std::vector<std::pair<std::string, std::string>> listCoreMemoryBlocks(int userId);
    bool updateCoreMemoryBlock(int userId, const std::string& label, 
                              const std::string& value, const std::string& encryptedValue);
    bool deleteCoreMemoryBlock(int userId, const std::string& label);
    
    // 添加查询功能
    std::vector<CoreMemoryBlockRecord> getAllCoreMemoryBlocks(int userId);
    CoreMemoryBlockRecord getCoreMemoryBlockById(int id);
    std::vector<CoreMemoryBlockRecord> searchCoreMemoryBlocks(int userId, const std::string& keyword);
};

#endif // CORE_MEMORY_BLOCK_MANAGER_H