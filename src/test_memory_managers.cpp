#include <iostream>
#include <string>
#include <vector>
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "database/managers/EpisodicMemoryManager.h"
#include "database/managers/SemanticMemoryManager.h"
#include "database/managers/ProceduralMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/managers/KnowledgeVaultManager.h"
#include "database/managers/ArchivalMemoryManager.h"
#include "database/managers/RecallMemoryManager.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"

void testCoreMemoryBlockManager(const std::string& dbPath) {
    std::cout << "=== 测试核心记忆管理器 ===" << std::endl;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    CoreMemoryBlockManager coreMemoryManager(storage);
    
    // 添加核心记忆
    std::cout << "1. 添加核心记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool coreMemoryAdded = coreMemoryManager.addCoreMemoryBlock(1, "test_key", "test_value", "Test core memory");
    if (coreMemoryAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   核心记忆添加成功" << std::endl;
    } else {
        std::cerr << "   核心记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询核心记忆
    std::cout << "2. 查询核心记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<std::pair<std::string, std::string>> coreMemories = coreMemoryManager.listCoreMemoryBlocks(1);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   用户核心记忆列表:" << std::endl;
    for (const auto& memory : coreMemories) {
        std::cout << "     " << memory.first << ": " << memory.second << std::endl;
    }
    
    std::cout << std::endl;
    
    // 测试getAllCoreMemoryBlocks功能
    std::cout << "3. 测试getAllCoreMemoryBlocks功能..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    auto allRecords = coreMemoryManager.getAllCoreMemoryBlocks(1);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有核心记忆记录:" << std::endl;
    for (const auto& record : allRecords) {
        std::cout << "     ID: " << record.id << ", Label: " << record.label 
                  << ", Value: " << record.value << std::endl;
    }
    
    std::cout << std::endl;
    
    // 测试searchCoreMemoryBlocks功能
    std::cout << "4. 测试searchCoreMemoryBlocks功能..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    auto searchResults = coreMemoryManager.searchCoreMemoryBlocks(1, "test");
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& record : searchResults) {
        std::cout << "     ID: " << record.id << ", Label: " << record.label 
                  << ", Value: " << record.value << std::endl;
    }
    
    std::cout << std::endl;
}

void testEpisodicMemoryManager(const std::string& dbPath) {
    std::cout << "\n=== 测试情景记忆管理器 ===" << std::endl;
    
    // 添加情景记忆
    std::cout << "1. 添加情景记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool episodicAdded = EpisodicMemoryManager::addEpisodicMemory(1, 1, "2023-01-01 00:00:00", 
        "test_event", "This is a test episodic memory.", "event", "/test", "{}", dbPath);
    if (episodicAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   情景记忆添加成功" << std::endl;
    } else {
        std::cerr << "   情景记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有情景记忆
    std::cout << "2. 查询所有情景记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<EpisodicMemoryRecord> allEpisodicMemories = EpisodicMemoryManager::getAllEpisodicMemories(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有情景记忆:" << std::endl;
    for (const auto& memory : allEpisodicMemories) {
        std::cout << "     ID: " << memory.id << ", Event: " << memory.event << ", Details: " << memory.details << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询情景记忆
    std::cout << "3. 根据ID查询情景记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    EpisodicMemoryRecord episodicMemory = EpisodicMemoryManager::getEpisodicMemoryById(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    if (episodicMemory.id > 0) {
        std::cout << "     ID: " << episodicMemory.id << ", Event: " << episodicMemory.event << ", Details: " << episodicMemory.details << std::endl;
    } else {
        std::cout << "     未找到ID为1的情景记忆" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索情景记忆
    std::cout << "4. 搜索情景记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<EpisodicMemoryRecord> searchResults = EpisodicMemoryManager::searchEpisodicMemories(1, "test", dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& memory : searchResults) {
        std::cout << "     ID: " << memory.id << ", Event: " << memory.event << ", Details: " << memory.details << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新情景记忆
    std::cout << "5. 更新情景记忆..." << std::endl;
    std::cout << "   操作前: summary=test_event, details=This is a test episodic memory." << std::endl;
    bool episodicUpdated = EpisodicMemoryManager::updateEpisodicMemory(1, "updated_event", 
        "This is an updated episodic memory.", "{}", dbPath);
    if (episodicUpdated) {
        std::cout << "   操作后: summary=updated_event, details=This is an updated episodic memory." << std::endl;
        std::cout << "   情景记忆更新成功" << std::endl;
    } else {
        std::cerr << "   情景记忆更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除情景记忆
    std::cout << "6. 删除情景记忆..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool episodicDeleted = EpisodicMemoryManager::deleteEpisodicMemory(1, dbPath);
    if (episodicDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   情景记忆删除成功" << std::endl;
    } else {
        std::cerr << "   情景记忆删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "情景记忆管理器测试完成" << std::endl;
}

void testSemanticMemoryManager(const std::string& dbPath) {
    std::cout << "\n=== 测试语义记忆管理器 ===" << std::endl;
    
    // 添加语义记忆
    std::cout << "1. 添加语义记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool semanticAdded = SemanticMemoryManager::addSemanticMemory(1, 1, "test_fact", 
        "test_fact", "This is a test semantic memory.", "/test", "{}", dbPath);
    if (semanticAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   语义记忆添加成功" << std::endl;
    } else {
        std::cerr << "   语义记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有语义记忆
    std::cout << "2. 查询所有语义记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<SemanticMemoryRecord> allSemanticMemories = SemanticMemoryManager::getAllSemanticMemories(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有语义记忆:" << std::endl;
    for (const auto& memory : allSemanticMemories) {
        std::cout << "     ID: " << memory.id << ", Name: " << memory.name << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询语义记忆
    std::cout << "3. 根据ID查询语义记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    SemanticMemoryRecord semanticMemory = SemanticMemoryManager::getSemanticMemoryById(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    if (semanticMemory.id > 0) {
        std::cout << "     ID: " << semanticMemory.id << ", Name: " << semanticMemory.name << ", Summary: " << semanticMemory.summary << std::endl;
    } else {
        std::cout << "     未找到ID为1的语义记忆" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索语义记忆
    std::cout << "4. 搜索语义记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<SemanticMemoryRecord> searchResults = SemanticMemoryManager::searchSemanticMemories(1, "test", dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& memory : searchResults) {
        std::cout << "     ID: " << memory.id << ", Name: " << memory.name << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新语义记忆
    std::cout << "5. 更新语义记忆..." << std::endl;
    std::cout << "   操作前: name=test_fact, summary=test_fact, details=This is a test semantic memory." << std::endl;
    bool semanticUpdated = SemanticMemoryManager::updateSemanticMemory(1, "updated_fact", 
        "updated_value", "This is an updated semantic memory.", "{}", dbPath);
    if (semanticUpdated) {
        std::cout << "   操作后: name=updated_fact, summary=updated_value, details=This is an updated semantic memory." << std::endl;
        std::cout << "   语义记忆更新成功" << std::endl;
    } else {
        std::cerr << "   语义记忆更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除语义记忆
    std::cout << "6. 删除语义记忆..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool semanticDeleted = SemanticMemoryManager::deleteSemanticMemory(1, dbPath);
    if (semanticDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   语义记忆删除成功" << std::endl;
    } else {
        std::cerr << "   语义记忆删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "语义记忆管理器测试完成" << std::endl;
}

void testProceduralMemoryManager(const std::string& dbPath) {
    std::cout << "\n=== 测试程序性记忆管理器 ===" << std::endl;
    
    // 添加程序性记忆
    std::cout << "1. 添加程序性记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool proceduralAdded = ProceduralMemoryManager::addProceduralMemory(1, 1, "test_skill", 
        "test_skill", "How to perform a test skill", "/test", "{}", dbPath);
    if (proceduralAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   程序性记忆添加成功" << std::endl;
    } else {
        std::cerr << "   程序性记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有程序性记忆
    std::cout << "2. 查询所有程序性记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ProceduralMemoryRecord> allProceduralMemories = ProceduralMemoryManager::getAllProceduralMemories(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有程序性记忆:" << std::endl;
    for (const auto& memory : allProceduralMemories) {
        std::cout << "     ID: " << memory.id << ", Title: " << memory.title << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询程序性记忆
    std::cout << "3. 根据ID查询程序性记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    ProceduralMemoryRecord proceduralMemory = ProceduralMemoryManager::getProceduralMemoryById(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    if (proceduralMemory.id > 0) {
        std::cout << "     ID: " << proceduralMemory.id << ", Title: " << proceduralMemory.title << ", Summary: " << proceduralMemory.summary << std::endl;
    } else {
        std::cout << "     未找到ID为1的程序性记忆" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索程序性记忆
    std::cout << "4. 搜索程序性记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ProceduralMemoryRecord> searchResults = ProceduralMemoryManager::searchProceduralMemories(1, "test", dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& memory : searchResults) {
        std::cout << "     ID: " << memory.id << ", Title: " << memory.title << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新程序性记忆
    std::cout << "5. 更新程序性记忆..." << std::endl;
    std::cout << "   操作前: title=test_skill, summary=test_skill, steps=How to perform a test skill" << std::endl;
    bool proceduralUpdated = ProceduralMemoryManager::updateProceduralMemory(1, "updated_skill", 
        "How to perform an updated skill", "updated_skill", "{}", dbPath);
    if (proceduralUpdated) {
        std::cout << "   操作后: title=updated_skill, summary=How to perform an updated skill, steps=updated_skill" << std::endl;
        std::cout << "   程序性记忆更新成功" << std::endl;
    } else {
        std::cerr << "   程序性记忆更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除程序性记忆
    std::cout << "6. 删除程序性记忆..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool proceduralDeleted = ProceduralMemoryManager::deleteProceduralMemory(1, dbPath);
    if (proceduralDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   程序性记忆删除成功" << std::endl;
    } else {
        std::cerr << "   程序性记忆删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "程序性记忆管理器测试完成" << std::endl;
}

void testResourceMemoryManager(const std::string& dbPath) {
    std::cout << "\n=== 测试资源记忆管理器 ===" << std::endl;
    
    // 添加资源记忆
    std::cout << "1. 添加资源记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool resourceAdded = ResourceMemoryManager::addResourceMemory(1, 1, "test_resource.txt", 
        "text/plain", "Test resource file", "Test content", 1024, "/test", "{}", dbPath);
    if (resourceAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   资源记忆添加成功" << std::endl;
    } else {
        std::cerr << "   资源记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有资源记忆
    std::cout << "2. 查询所有资源记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ResourceMemoryRecord> allResourceMemories = ResourceMemoryManager::getAllResourceMemories(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有资源记忆:" << std::endl;
    for (const auto& memory : allResourceMemories) {
        std::cout << "     ID: " << memory.id << ", Filename: " << memory.filename << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询资源记忆
    std::cout << "3. 根据ID查询资源记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    ResourceMemoryRecord resourceMemory = ResourceMemoryManager::getResourceMemoryById(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    if (resourceMemory.id > 0) {
        std::cout << "     ID: " << resourceMemory.id << ", Filename: " << resourceMemory.filename << ", Summary: " << resourceMemory.summary << std::endl;
    } else {
        std::cout << "     未找到ID为1的资源记忆" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索资源记忆
    std::cout << "4. 搜索资源记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ResourceMemoryRecord> searchResults = ResourceMemoryManager::searchResourceMemories(1, "test", dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& memory : searchResults) {
        std::cout << "     ID: " << memory.id << ", Filename: " << memory.filename << ", Summary: " << memory.summary << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新资源记忆
    std::cout << "5. 更新资源记忆..." << std::endl;
    std::cout << "   操作前: summary=Test resource file, content=Test content" << std::endl;
    bool resourceUpdated = ResourceMemoryManager::updateResourceMemory(1, "Updated resource file", 
        "Updated content", "{}", dbPath);
    if (resourceUpdated) {
        std::cout << "   操作后: summary=Updated resource file, content=Updated content" << std::endl;
        std::cout << "   资源记忆更新成功" << std::endl;
    } else {
        std::cerr << "   资源记忆更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除资源记忆
    std::cout << "6. 删除资源记忆..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool resourceDeleted = ResourceMemoryManager::deleteResourceMemory(1, dbPath);
    if (resourceDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   资源记忆删除成功" << std::endl;
    } else {
        std::cerr << "   资源记忆删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "资源记忆管理器测试完成" << std::endl;
}

void testKnowledgeVaultManager(const std::string& dbPath) {
    std::cout << "\n=== 测试知识库管理器 ===" << std::endl;
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    KnowledgeVaultManager knowledgeVaultManager(storage);
    
    // 添加知识库条目
    std::cout << "1. 添加知识库条目..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool knowledgeAdded = knowledgeVaultManager.addKnowledgeVaultEntry(1, 1, "test_knowledge", 
        "general", "Test source", "public", "Test knowledge entry", "{}");
    if (knowledgeAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   知识库条目添加成功" << std::endl;
    } else {
        std::cerr << "   知识库条目添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有知识库条目
    std::cout << "2. 查询所有知识库条目..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<KnowledgeVaultRecord> allKnowledgeVaultEntries = knowledgeVaultManager.getAllKnowledgeVaultEntries(1);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有知识库条目:" << std::endl;
    for (const auto& entry : allKnowledgeVaultEntries) {
        std::cout << "     ID: " << entry.id << ", Caption: " << entry.caption << ", Type: " << entry.entry_type << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询知识库条目
    std::cout << "3. 根据ID查询知识库条目..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    KnowledgeVaultRecord knowledgeVaultEntry = knowledgeVaultManager.getKnowledgeVaultEntryById(1);
    std::cout << "   操作后:" << std::endl;
    if (knowledgeVaultEntry.id > 0) {
        std::cout << "     ID: " << knowledgeVaultEntry.id << ", Caption: " << knowledgeVaultEntry.caption << ", Type: " << knowledgeVaultEntry.entry_type << std::endl;
    } else {
        std::cout << "     未找到ID为1的知识库条目" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索知识库条目
    std::cout << "4. 搜索知识库条目..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<KnowledgeVaultRecord> searchResults = knowledgeVaultManager.searchKnowledgeVaultEntries(1, "test");
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& entry : searchResults) {
        std::cout << "     ID: " << entry.id << ", Caption: " << entry.caption << ", Type: " << entry.entry_type << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新知识库条目
    std::cout << "5. 更新知识库条目..." << std::endl;
    std::cout << "   操作前: caption=test_knowledge, entry_type=general, source=Test source" << std::endl;
    bool knowledgeUpdated = knowledgeVaultManager.updateKnowledgeVaultEntry(1, "updated_knowledge", 
        "general", "Updated source", "public", "Updated knowledge entry", "{}");
    if (knowledgeUpdated) {
        std::cout << "   操作后: caption=updated_knowledge, entry_type=general, source=Updated source" << std::endl;
        std::cout << "   知识库条目更新成功" << std::endl;
    } else {
        std::cerr << "   知识库条目更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除知识库条目
    std::cout << "6. 删除知识库条目..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool knowledgeDeleted = knowledgeVaultManager.deleteKnowledgeVaultEntry(1);
    if (knowledgeDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   知识库条目删除成功" << std::endl;
    } else {
        std::cerr << "   知识库条目删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "知识库管理器测试完成" << std::endl;
}

void testArchivalMemoryManager(const std::string& dbPath) {
    std::cout << "\n=== 测试归档记忆管理器 ===" << std::endl;
    
    // 添加归档记忆
    std::cout << "1. 添加归档记忆..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    bool archivalAdded = ArchivalMemoryManager::addArchivalMemory(1, 1, 
        "test", "This is a test archival memory", "{}", dbPath);
    if (archivalAdded) {
        std::cout << "   操作后: 添加成功" << std::endl;
        std::cout << "   归档记忆添加成功" << std::endl;
    } else {
        std::cerr << "   归档记忆添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 查询所有归档记忆
    std::cout << "2. 查询所有归档记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ArchivalMemoryRecord> allArchivalMemories = ArchivalMemoryManager::getAllArchivalMemories(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   所有归档记忆:" << std::endl;
    for (const auto& memory : allArchivalMemories) {
        std::cout << "     ID: " << memory.id << ", Content: " << memory.content << std::endl;
    }
    
    std::cout << std::endl;
    
    // 根据ID查询归档记忆
    std::cout << "3. 根据ID查询归档记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    ArchivalMemoryRecord archivalMemory = ArchivalMemoryManager::getArchivalMemoryById(1, dbPath);
    std::cout << "   操作后:" << std::endl;
    if (archivalMemory.id > 0) {
        std::cout << "     ID: " << archivalMemory.id << ", Content: " << archivalMemory.content << std::endl;
    } else {
        std::cout << "     未找到ID为1的归档记忆" << std::endl;
    }
    
    std::cout << std::endl;
    
    // 搜索归档记忆
    std::cout << "4. 搜索归档记忆..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    std::vector<ArchivalMemoryRecord> searchResults = ArchivalMemoryManager::searchArchivalMemories(1, "test", dbPath);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& memory : searchResults) {
        std::cout << "     ID: " << memory.id << ", Content: " << memory.content << std::endl;
    }
    
    std::cout << std::endl;
    
    // 更新归档记忆
    std::cout << "5. 更新归档记忆..." << std::endl;
    std::cout << "   操作前: content=This is a test archival memory" << std::endl;
    bool archivalUpdated = ArchivalMemoryManager::updateArchivalMemory(1, 
        "This is an updated archival memory", "{}", dbPath);
    if (archivalUpdated) {
        std::cout << "   操作后: content=This is an updated archival memory" << std::endl;
        std::cout << "   归档记忆更新成功" << std::endl;
    } else {
        std::cerr << "   归档记忆更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 删除归档记忆
    std::cout << "6. 删除归档记忆..." << std::endl;
    std::cout << "   操作前: 存在ID为1的记录" << std::endl;
    bool archivalDeleted = ArchivalMemoryManager::deleteArchivalMemory(1, dbPath);
    if (archivalDeleted) {
        std::cout << "   操作后: ID为1的记录已删除" << std::endl;
        std::cout << "   归档记忆删除成功" << std::endl;
    } else {
        std::cerr << "   归档记忆删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    std::cout << "归档记忆管理器测试完成" << std::endl;
}

void testMemoryManagers() {
    std::cout << "=== 测试记忆管理器 ===" << std::endl;
    
    std::string dbPath = "test_memory_managers.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    // 添加用户
    bool userAdded = UserManager::addUser("testuser", "test@example.com", "", dbPath);
    if (!userAdded) {
        std::cerr << "添加用户失败!" << std::endl;
        return;
    }
    std::cout << "添加用户成功" << std::endl;
    
    // 获取用户ID
    int userId = UserManager::getUserIdByName("testuser", dbPath);
    if (userId <= 0) {
        std::cerr << "获取用户ID失败!" << std::endl;
        return;
    }

    // 测试会话管理器
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = std::to_string(userId);
    sessionItem.start_time = "2023-01-01 00:00:00";
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.metadata = "{}";
    
    bool sessionCreated = sessionManager.create_session(sessionItem);
    int sessionId = 1;
    if (!sessionCreated) {
        std::cerr << "创建会话失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 测试各个记忆管理器
    testCoreMemoryBlockManager(dbPath);
    testEpisodicMemoryManager(dbPath);
    testSemanticMemoryManager(dbPath);
    testProceduralMemoryManager(dbPath);
    testResourceMemoryManager(dbPath);
    testKnowledgeVaultManager(dbPath);
    testArchivalMemoryManager(dbPath);
    
    std::cout << "\n记忆管理器测试完成" << std::endl;
}

int main() {
    std::cout << "开始测试记忆管理器..." << std::endl;
    
    testMemoryManagers();
    
    std::cout << "\n记忆管理器测试完成!" << std::endl;
    return 0;
}