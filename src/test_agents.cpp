#include <iostream>
#include <string>
#include "database/core/StorageManager.h"
#include "database/initialization/InitDB.h"
#include "database/managers/CoreMemoryBlockManager.h"
#include "database/managers/EpisodicMemoryManager.h"
#include "database/managers/SemanticMemoryManager.h"
#include "database/managers/ProceduralMemoryManager.h"
#include "database/managers/ResourceMemoryManager.h"
#include "database/managers/KnowledgeVaultManager.h"
#include "database/managers/ArchivalMemoryManager.h"
#include "database/managers/UserManager.h"
#include "database/managers/SessionManager.h"
#include "database/managers/AuditLogManager.h"
#include "database/managers/BackupManager.h"
#include "database/managers/MemoryOrchestrator.h"
#include "database/initialization/ConfigManager/ConfigManager.h"
#include "models/ModelManager.h"
#include "models/EmbeddingIndexManager.h"
#include "agents/AgentWrapper.h"
#include "agents/AgentManager.h"
#include "agents/ChatAgent.h"
#include "agents/CoreMemoryAgent.h"
#include "agents/EpisodicMemoryAgent.h"
#include "agents/SemanticMemoryAgent.h"
#include "agents/ProceduralMemoryAgent.h"
#include "agents/ResourceMemoryAgent.h"
#include "agents/KnowledgeVaultAgent.h"

void testAgentManager() {
    std::cout << "=== 测试代理管理器 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    // 测试用户管理器
    bool userAdded = UserManager::addUser("testuser", "test@example.com", "", dbPath);
    int userId = 1;
    if (!userAdded) {
        std::cerr << "创建用户失败!" << std::endl;
        return;
    }
    
    // 测试会话管理器
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
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
    
    // 测试模型管理器
    ModelManager modelManager(storage);
    
    // 测试代理管理器
    AgentManager agentManager(storage, modelManager);
    AgentWrapper* agentWrapper = agentManager.get_agent_wrapper();
    if (agentWrapper) {
        std::cout << "代理管理器初始化成功" << std::endl;
    } else {
        std::cerr << "代理管理器初始化失败!" << std::endl;
    }
    
    std::cout << "代理管理器测试完成" << std::endl;
}

void testChatAgent() {
    std::cout << "=== 测试聊天代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    ModelManager modelManager(storage);
    ChatAgent chatAgent(storage, modelManager);
    
    std::cout << "聊天代理初始化成功" << std::endl;
    std::cout << "聊天代理测试完成" << std::endl;
}

void testCoreMemoryAgent() {
    std::cout << "=== 测试核心记忆代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    CoreMemoryAgent coreMemoryAgent(storage);
    
    std::cout << "核心记忆代理初始化成功" << std::endl;
    std::cout << "核心记忆代理测试完成" << std::endl;
}

void testEpisodicMemoryAgent() {
    std::cout << "=== 测试情景记忆代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    EpisodicMemoryAgent episodicMemoryAgent(storage);
    
    std::cout << "情景记忆代理初始化成功" << std::endl;
    std::cout << "情景记忆代理测试完成" << std::endl;
}

void testSemanticMemoryAgent() {
    std::cout << "=== 测试语义记忆代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    SemanticMemoryAgent semanticMemoryAgent(storage);
    
    std::cout << "语义记忆代理初始化成功" << std::endl;
    std::cout << "语义记忆代理测试完成" << std::endl;
}

void testProceduralMemoryAgent() {
    std::cout << "=== 测试程序性记忆代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    ProceduralMemoryAgent proceduralMemoryAgent(storage);
    
    std::cout << "程序性记忆代理初始化成功" << std::endl;
    std::cout << "程序性记忆代理测试完成" << std::endl;
}

void testResourceMemoryAgent() {
    std::cout << "=== 测试资源记忆代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    
    // 初始化数据库
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    ResourceMemoryAgent resourceMemoryAgent(storage);
    
    std::cout << "资源记忆代理初始化成功" << std::endl;
    std::cout << "资源记忆代理测试完成" << std::endl;
}

void testKnowledgeVaultAgent() {
    std::cout << "=== 测试知识库代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "存储管理器初始化失败!" << std::endl;
        return;
    }
    
    KnowledgeVaultAgent knowledgeVaultAgent(storage);
    
    // 测试添加知识条目
    std::cout << "1. 添加知识条目..." << std::endl;
    std::cout << "   操作前: 无记录" << std::endl;
    int entryId = knowledgeVaultAgent.add_knowledge_entry(
        1, 
        "Test Title", 
        "Test Content", 
        "general", 
        "test,tags", 
        "test_source", 
        "{\"key\":\"value\"}"
    );
    if (entryId > 0) {
        std::cout << "   操作后: 添加成功，ID=" << entryId << std::endl;
        std::cout << "   知识条目添加成功" << std::endl;
    } else {
        std::cerr << "   知识条目添加失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 测试列出知识条目
    std::cout << "2. 列出知识条目..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    auto entries = knowledgeVaultAgent.list_knowledge_entries(1, 10);
    std::cout << "   操作后:" << std::endl;
    std::cout << "   知识条目列表:" << std::endl;
    for (const auto& entry : entries) {
        std::cout << "     ID: " << entry.id << ", Title: " << entry.title 
                  << ", Content: " << entry.content << std::endl;
    }
    
    std::cout << std::endl;
    
    // 测试更新知识条目
    std::cout << "3. 更新知识条目..." << std::endl;
    std::cout << "   操作前: title=Test Title, content=Test Content" << std::endl;
    bool updated = knowledgeVaultAgent.update_knowledge_entry(
        entryId, 
        "Updated Title", 
        "Updated Content", 
        "updated_category", 
        "updated,tags", 
        "updated_source", 
        "{\"updated\":\"true\"}"
    );
    if (updated) {
        std::cout << "   操作后: title=Updated Title, content=Updated Content" << std::endl;
        std::cout << "   知识条目更新成功" << std::endl;
    } else {
        std::cerr << "   知识条目更新失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    // 测试搜索知识条目
    std::cout << "4. 搜索知识条目..." << std::endl;
    std::cout << "   操作前: 无显示" << std::endl;
    auto searchResults = knowledgeVaultAgent.search_knowledge_entries(1, "Updated");
    std::cout << "   操作后:" << std::endl;
    std::cout << "   搜索结果:" << std::endl;
    for (const auto& entry : searchResults) {
        std::cout << "     ID: " << entry.id << ", Title: " << entry.title << std::endl;
    }
    
    std::cout << std::endl;
    
    // 测试删除知识条目
    std::cout << "5. 删除知识条目..." << std::endl;
    std::cout << "   操作前: 存在ID为" << entryId << "的记录" << std::endl;
    bool deleted = knowledgeVaultAgent.delete_knowledge_entry(entryId);
    if (deleted) {
        auto remainingEntries = knowledgeVaultAgent.list_knowledge_entries(1, 10);
        std::cout << "   操作后: ID为" << entryId << "的记录已删除，剩余条目数: " 
                  << remainingEntries.size() << std::endl;
        std::cout << "   知识条目删除成功" << std::endl;
    } else {
        std::cerr << "   知识条目删除失败!" << std::endl;
        return;
    }
    
    std::cout << std::endl;
    
    std::cout << "知识库代理初始化成功" << std::endl;
    std::cout << "知识库代理测试完成" << std::endl << std::endl;
}

void testAgents() {
    std::cout << "=== 测试代理 ===" << std::endl;
    
    std::string dbPath = "test_agents.db";
    StorageManager storage(dbPath);
    
    if (!storage.initialize()) {
        std::cerr << "数据库初始化失败!" << std::endl;
        return;
    }
    
    if (!InitDB::initializeDatabase(dbPath)) {
        std::cerr << "数据库表初始化失败!" << std::endl;
        return;
    }
    
    // 测试用户管理器
    bool userAdded = UserManager::addUser("testuser", "test@example.com", dbPath);
    if (!userAdded) {
        std::cerr << "添加用户失败!" << std::endl;
        return;
    }
    
    int userId = UserManager::getUserIdByName("testuser", dbPath);
    if (userId <= 0) {
        std::cerr << "获取用户ID失败!" << std::endl;
        return;
    }
    
    // 测试会话管理器
    SessionManager sessionManager(storage);
    SessionItem sessionItem;
    sessionItem.user_id = std::to_string(userId);
    sessionItem.start_time = "2023-01-01 00:00:00";
    sessionItem.end_time = "";
    sessionItem.status = "active";
    sessionItem.metadata = "{}";
    
    bool sessionCreated = sessionManager.create_session(sessionItem);
    int sessionId = 1;
    if (sessionCreated) {
        std::cout << "创建会话成功，会话ID: " << sessionId << std::endl;
    } else {
        std::cerr << "创建会话失败!" << std::endl;
        return;
    }
    
    // 测试聊天代理
    // 修复：只传递StorageManager参数
    ChatAgent chatAgent(storage);
    chatAgent.initialize(userId, 1);
    
    // 测试核心记忆代理
    CoreMemoryAgent coreMemoryAgent(storage);
    coreMemoryAgent.initialize(userId, 1);
    
    // 测试情景记忆代理
    EpisodicMemoryAgent episodicMemoryAgent(storage);
    episodicMemoryAgent.initialize(userId, 1);
    
    // 测试语义记忆代理
    SemanticMemoryAgent semanticMemoryAgent(storage);
    semanticMemoryAgent.initialize(userId, 1);
    
    // 测试程序性记忆代理
    ProceduralMemoryAgent proceduralMemoryAgent(storage);
    proceduralMemoryAgent.initialize(userId, 1);
    
    // 测试资源记忆代理
    ResourceMemoryAgent resourceMemoryAgent(storage);
    resourceMemoryAgent.initialize(userId, 1);
    
    // 测试知识库代理
    KnowledgeVaultAgent knowledgeVaultAgent(storage);
    knowledgeVaultAgent.initialize(userId, 1);
    
    // 测试代理管理器
    AgentManager agentManager(storage);
    agentManager.registerAgent("chat", &chatAgent);
    agentManager.registerAgent("core_memory", &coreMemoryAgent);
    agentManager.registerAgent("episodic_memory", &episodicMemoryAgent);
    agentManager.registerAgent("semantic_memory", &semanticMemoryAgent);
    agentManager.registerAgent("procedural_memory", &proceduralMemoryAgent);
    agentManager.registerAgent("resource_memory", &resourceMemoryAgent);
    agentManager.registerAgent("knowledge_vault", &knowledgeVaultAgent);
    
    // 测试聊天功能
    std::cout << "\n--- 测试聊天功能 ---" << std::endl;
    std::string chatResponse = chatAgent.chat(userId, 1, sessionId, "你好，这是一个测试消息");
    std::cout << "聊天响应: " << chatResponse << std::endl;
    
    // 测试核心记忆功能
    std::cout << "\n--- 测试核心记忆功能 ---" << std::endl;
    bool coreMemoryAdded = coreMemoryAgent.addCoreMemory("test_key", "test_value", "测试核心记忆");
    std::cout << "添加核心记忆: " << (coreMemoryAdded ? "成功" : "失败") << std::endl;
    
    auto coreMemories = coreMemoryAgent.listCoreMemoryBlocks();
    std::cout << "核心记忆列表:" << std::endl;
    for (const auto& pair : coreMemories) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 测试情景记忆功能
    std::cout << "\n--- 测试情景记忆功能 ---" << std::endl;
    bool episodicMemoryAdded = episodicMemoryAgent.addEpisodicMemory(
        "测试事件", 
        "这是一个测试事件",
        "test_event", 
        "/test/event", 
        "{}"
    );
    std::cout << "添加情景记忆: " << (episodicMemoryAdded ? "成功" : "失败") << std::endl;
    
    auto episodicMemories = episodicMemoryAgent.listEpisodicMemories();
    std::cout << "情景记忆列表:" << std::endl;
    for (const auto& memory : episodicMemories) {
        std::cout << "  ID: " << memory.id 
                  << ", 内容: " << memory.content 
                  << ", 类型: " << memory.event_type << std::endl;
    }
    
    // 测试语义记忆功能
    std::cout << "\n--- 测试语义记忆功能 ---" << std::endl;
    bool semanticMemoryAdded = semanticMemoryAgent.addSemanticMemory(
        "测试事实", 
        "这是一个测试事实",
        "测试事实的详细描述",
        "/test/fact", 
        "{}"
    );
    std::cout << "添加语义记忆: " << (semanticMemoryAdded ? "成功" : "失败") << std::endl;
    
    auto semanticMemories = semanticMemoryAgent.listSemanticMemories();
    std::cout << "语义记忆列表:" << std::endl;
    for (const auto& memory : semanticMemories) {
        std::cout << "  ID: " << memory.id 
                  << ", 标题: " << memory.title 
                  << ", 内容: " << memory.content << std::endl;
    }
    
    // 测试程序性记忆功能
    std::cout << "\n--- 测试程序性记忆功能 ---" << std::endl;
    bool proceduralMemoryAdded = proceduralMemoryAgent.addProceduralMemory(
        "测试技能", 
        "测试技能的描述",
        "测试技能的详细步骤",
        "/test/skill", 
        "{}"
    );
    std::cout << "添加程序性记忆: " << (proceduralMemoryAdded ? "成功" : "失败") << std::endl;
    
    auto proceduralMemories = proceduralMemoryAgent.listProceduralMemories();
    std::cout << "程序性记忆列表:" << std::endl;
    for (const auto& memory : proceduralMemories) {
        std::cout << "  ID: " << memory.id 
                  << ", 标题: " << memory.title 
                  << ", 描述: " << memory.description << std::endl;
    }
    
    // 测试资源记忆功能
    std::cout << "\n--- 测试资源记忆功能 ---" << std::endl;
    bool resourceMemoryAdded = resourceMemoryAgent.addResourceMemory(
        "测试资源", 
        "这是一个测试资源",
        "test_resource", 
        "/test/resource", 
        "{}"
    );
    std::cout << "添加资源记忆: " << (resourceMemoryAdded ? "成功" : "失败") << std::endl;
    
    auto resourceMemories = resourceMemoryAgent.listResourceMemories();
    std::cout << "资源记忆列表:" << std::endl;
    for (const auto& memory : resourceMemories) {
        std::cout << "  ID: " << memory.id 
                  << ", 标题: " << memory.title 
                  << ", 类型: " << memory.resource_type << std::endl;
    }
    
    // 测试知识库功能
    std::cout << "\n--- 测试知识库功能 ---" << std::endl;
    bool knowledgeAdded = knowledgeVaultAgent.addKnowledge(
        "测试知识", 
        "这是测试知识的内容",
        "test_knowledge", 
        "/test/knowledge", 
        "{}"
    );
    std::cout << "添加知识: " << (knowledgeAdded ? "成功" : "失败") << std::endl;
    
    auto knowledgeList = knowledgeVaultAgent.listKnowledge();
    std::cout << "知识列表:" << std::endl;
    for (const auto& knowledge : knowledgeList) {
        std::cout << "  ID: " << knowledge.id 
                  << ", 标题: " << knowledge.title 
                  << ", 类型: " << knowledge.category << std::endl;
    }
    
    std::cout << "\n代理测试完成" << std::endl;
}

void testAgents() {
    std::cout << "=== 测试代理 ===" << std::endl;
    
    testAgentManager();
    testChatAgent();
    testCoreMemoryAgent();
    testEpisodicMemoryAgent();
    testSemanticMemoryAgent();
    testProceduralMemoryAgent();
    testResourceMemoryAgent();
    testKnowledgeVaultAgent();
    
    std::cout << "代理测试完成" << std::endl;
}

int main() {
    std::cout << "开始测试代理..." << std::endl;
    
    testAgents();
    
    std::cout << "代理测试完成!" << std::endl;
    return 0;
}