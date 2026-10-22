#include "infra/storage/StorageManager.h"
#include "legacy/core-v1/UserManager.h"
#include "legacy/core-v1/CoreMemoryBlockManager.h"
#include "legacy/core-v1/EpisodicMemoryManager.h"
#include "legacy/core-v1/SemanticMemoryManager.h"
#include "legacy/core-v1/ProceduralMemoryManager.h"
#include "legacy/core-v1/ResourceMemoryManager.h"
#include "legacy/core-v1/KnowledgeVaultManager.h"
#include "legacy/core-v1/ArchivalMemoryManager.h"
#include <iostream>
#include <vector>
#include <string>

void initializeMemories(int userId, const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return;
    }
    
    // 初始化核心记忆
    CoreMemoryBlockManager coreMemoryManager(storage);
    bool core1_added = coreMemoryManager.addCoreMemoryBlock(userId, "user_name", "Alice", "");
    if (core1_added) {
        std::cout << "Added core memory user_name" << std::endl;
    }
    
    bool core2_added = coreMemoryManager.addCoreMemoryBlock(userId, "user_role", "developer", "");
    if (core2_added) {
        std::cout << "Added core memory user_role" << std::endl;
    }
    
    // 初始化情景记忆
    bool episodic1_added = EpisodicMemoryManager::addEpisodicMemory(
        userId, 1, 
        "2023-01-01 10:00:00", 
        "System Installation", 
        "Installed MIRIX system version 1.0",
        "system_event",
        "/events/installation_001",
        "{\"version\": \"1.0\"}",
        dbPath
    );
    if (episodic1_added) {
        std::cout << "Added episodic memory event1" << std::endl;
    }
    
    bool episodic2_added = EpisodicMemoryManager::addEpisodicMemory(
        userId, 1, 
        "2023-01-02 14:30:00", 
        "First User Login", 
        "User Alice logged into the system for the first time",
        "user_event",
        "/events/login_001",
        "{\"user\": \"Alice\"}",
        dbPath
    );
    if (episodic2_added) {
        std::cout << "Added episodic memory event2" << std::endl;
    }
    
    // 初始化语义记忆
    bool semantic1_added = SemanticMemoryManager::addSemanticMemory(
        userId, 1,
        "System Architecture",
        "MIRIX is a multi-layered memory system with core, episodic, semantic, and procedural components",
        "System overview and architecture",
        "/docs/architecture",
        "{\"tags\": [\"system\", \"architecture\"]}",
        dbPath
    );
    if (semantic1_added) {
        std::cout << "Added semantic memory concept1" << std::endl;
    }
    
    bool semantic2_added = SemanticMemoryManager::addSemanticMemory(
        userId, 1,
        "Memory Types",
        "The system supports multiple memory types: core (immediate access), episodic (events), semantic (facts), procedural (skills)",
        "Classification and description of memory types",
        "/docs/memory_types",
        "{\"tags\": [\"memory\", \"classification\"]}",
        dbPath
    );
    if (semantic2_added) {
        std::cout << "Added semantic memory concept2" << std::endl;
    }
    
    // 初始化程序性记忆
    bool procedural1_added = ProceduralMemoryManager::addProceduralMemory(
        userId, 1,
        "Data Backup",
        "Procedure for backing up system data",
        "1. Navigate to System Settings\n2. Select Backup Options\n3. Choose backup location\n4. Click 'Start Backup'\n5. Confirm completion",
        "/procedures/backup",
        "{\"estimated_time\": \"5 minutes\"}",
        dbPath
    );
    if (procedural1_added) {
        std::cout << "Added procedural memory skill1" << std::endl;
    }
    
    // 初始化资源记忆
    ResourceMemoryManager resourceMemoryManager(storage);
    ResourceMemoryManager::ResourceMemoryItem item1;
    item1.user_id = userId;
    item1.agent_id = 1;
    item1.session_id = 1;
    item1.description = "system_manual.pdf: Detailed introduction to the usage methods and feature descriptions of the MIRIX system";  // 合并title和content为description
    item1.resource_type = "application/pdf";
    item1.size = 1024;
    item1.path = "/resources/system_manual.pdf";
    item1.metadata = "{}";
    
    bool resource1_added = resourceMemoryManager.add_resource_memory(item1);
    if (resource1_added) {
        std::cout << "Added resource memory file1" << std::endl;
    }
    
    // 初始化知识库
    KnowledgeVaultManager knowledgeVaultManager(storage);
    bool knowledge1_added = knowledgeVaultManager.addKnowledgeVaultEntry(
        userId, 1,
        "FAQ: System Requirements",
        "faq",
        "/knowledge/faq/system_requirements",
        "public",
        "What are the minimum system requirements for MIRIX?\n- CPU: 2GHz dual-core processor\n- RAM: 4GB\n- Storage: 10GB free space\n- OS: Windows 10 or macOS 10.14+",
        "{\"related\": [\"installation\"]}"
    );
    if (knowledge1_added) {
        std::cout << "Added knowledge vault entry1" << std::endl;
    }
    
    bool knowledge2_added = knowledgeVaultManager.addKnowledgeVaultEntry(
        userId, 1,
        "Troubleshooting: Database Connection",
        "troubleshooting",
        "/knowledge/troubleshooting/db_connection",
        "public",
        "If the system cannot connect to the database:\n1. Check if the database service is running\n2. Verify database credentials\n3. Ensure network connectivity\n4. Check firewall settings",
        "{\"severity\": \"medium\"}"
    );
    if (knowledge2_added) {
        std::cout << "Added knowledge vault entry2" << std::endl;
    }
    
    // 初始化归档记忆
    bool archival1_added = ArchivalMemoryManager::addArchivalMemory(
        userId, 1,
        "historical_event",
        "Initial system development milestone reached",
        "{\"date\": \"2023-01-01\"}",
        dbPath
    );
    if (archival1_added) {
        std::cout << "Added archival memory record1" << std::endl;
    }
    
    std::cout << "Memory initialization completed" << std::endl;
    
    // 测试查询功能
    std::cout << "\n=== Testing Query Functions ===" << std::endl;
    
    // 测试核心记忆查询
    std::vector<CoreMemoryBlockRecord> coreMemories = coreMemoryManager.getAllCoreMemoryBlocks(userId);
    std::cout << "Core memories count: " << coreMemories.size() << std::endl;
    if (!coreMemories.empty()) {
        CoreMemoryBlockRecord record = coreMemoryManager.getCoreMemoryBlockById(coreMemories[0].id);
        std::cout << "First core memory: " << record.label << " = " << record.value << std::endl;
    }
    
    // 测试情景记忆查询
    std::vector<EpisodicMemoryRecord> episodicMemories = EpisodicMemoryManager::getAllEpisodicMemories(userId, dbPath);
    std::cout << "Episodic memories count: " << episodicMemories.size() << std::endl;
    if (!episodicMemories.empty()) {
        EpisodicMemoryRecord record = EpisodicMemoryManager::getEpisodicMemoryById(episodicMemories[0].id, dbPath);
        std::cout << "First episodic memory: " << record.event << std::endl;
    }
    
    // 测试语义记忆查询
    std::vector<SemanticMemoryRecord> semanticMemories = SemanticMemoryManager::getAllSemanticMemories(userId, dbPath);
    std::cout << "Semantic memories count: " << semanticMemories.size() << std::endl;
    if (!semanticMemories.empty()) {
        SemanticMemoryRecord record = SemanticMemoryManager::getSemanticMemoryById(semanticMemories[0].id, dbPath);
        std::cout << "First semantic memory: " << record.name << std::endl;
    }
    
    // 测试程序性记忆查询
    std::vector<ProceduralMemoryRecord> proceduralMemories = ProceduralMemoryManager::getAllProceduralMemories(userId, dbPath);
    std::cout << "Procedural memories count: " << proceduralMemories.size() << std::endl;
    if (!proceduralMemories.empty()) {
        ProceduralMemoryRecord record = ProceduralMemoryManager::getProceduralMemoryById(proceduralMemories[0].id, dbPath);
        std::cout << "First procedural memory: " << record.title << std::endl;
    }
    
    // 测试资源记忆查询
    std::vector<ResourceMemoryManager::ResourceMemoryItem> resourceMemories = resourceMemoryManager.list_resource_memories(userId);
    std::cout << "Resource memories count: " << resourceMemories.size() << std::endl;
    if (!resourceMemories.empty()) {
        std::cout << "First resource memory: " << resourceMemories[0].path << " - " << resourceMemories[0].description << std::endl;
    }
    
    // 测试知识库查询
    std::vector<KnowledgeVaultRecord> knowledgeMemories = knowledgeVaultManager.getAllKnowledgeVaultEntries(userId);
    std::cout << "Knowledge vault entries count: " << knowledgeMemories.size() << std::endl;
    if (!knowledgeMemories.empty()) {
        KnowledgeVaultRecord record = knowledgeVaultManager.getKnowledgeVaultEntryById(knowledgeMemories[0].id);
        std::cout << "First knowledge vault entry: " << record.caption << std::endl;
    }
    
    // 测试归档记忆查询
    std::vector<ArchivalMemoryRecord> archivalMemories = ArchivalMemoryManager::getAllArchivalMemories(userId, dbPath);
    std::cout << "Archival memories count: " << archivalMemories.size() << std::endl;
    if (!archivalMemories.empty()) {
        ArchivalMemoryRecord record = ArchivalMemoryManager::getArchivalMemoryById(archivalMemories[0].id, dbPath);
        std::cout << "First archival memory: " << record.memory_type << std::endl;
    }
    
    // 测试搜索功能
    std::cout << "\n=== Testing Search Functions ===" << std::endl;
    
    // 测试核心记忆搜索
    std::vector<CoreMemoryBlockRecord> coreSearchResults = coreMemoryManager.searchCoreMemoryBlocks(userId, "user");
    std::cout << "Core memory search results for 'user': " << coreSearchResults.size() << std::endl;
    
    // 测试情景记忆搜索
    std::vector<EpisodicMemoryRecord> episodicSearchResults = EpisodicMemoryManager::searchEpisodicMemories(userId, "System", dbPath);
    std::cout << "Episodic memory search results for 'System': " << episodicSearchResults.size() << std::endl;
    
    // 测试语义记忆搜索
    std::vector<SemanticMemoryRecord> semanticSearchResults = SemanticMemoryManager::searchSemanticMemories(userId, "memory", dbPath);
    std::cout << "Semantic memory search results for 'memory': " << semanticSearchResults.size() << std::endl;
    
    // 测试程序性记忆搜索
    std::vector<ProceduralMemoryRecord> proceduralSearchResults = ProceduralMemoryManager::searchProceduralMemories(userId, "Backup", dbPath);
    std::cout << "Procedural memory search results for 'Backup': " << proceduralSearchResults.size() << std::endl;
    
    // 测试资源记忆搜索
    std::vector<ResourceMemoryManager::ResourceMemoryItem> resourceSearchResults = resourceMemoryManager.search_resource_memories(userId, "manual");
    std::cout << "Resource memory search results for 'manual': " << resourceSearchResults.size() << std::endl;
    
    // 测试知识库搜索
    std::vector<KnowledgeVaultRecord> knowledgeSearchResults = knowledgeVaultManager.searchKnowledgeVaultEntries(userId, "FAQ");
    std::cout << "Knowledge vault search results for 'FAQ': " << knowledgeSearchResults.size() << std::endl;
    
    // 测试归档记忆搜索
    std::vector<ArchivalMemoryRecord> archivalSearchResults = ArchivalMemoryManager::searchArchivalMemories(userId, "initial", dbPath);
    std::cout << "Archival memory search results for 'initial': " << archivalSearchResults.size() << std::endl;
}