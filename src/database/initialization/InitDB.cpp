#include "InitDB.h"
#include "../core/StorageManager.h"
#include <iostream>

bool InitDB::initializeDatabase(const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database connection" << std::endl;
        return false;
    }

    // 创建用户表
    std::string userTable = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            email TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!storage.executeNonQuery(userTable)) {
        return false;
    }

    // 创建代理表
    std::string agentTable = R"(
        CREATE TABLE IF NOT EXISTS agent (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            capabilities TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    if (!storage.executeNonQuery(agentTable)) {
        return false;
    }

    // 创建会话表
    std::string sessionTable = R"(
        CREATE TABLE IF NOT EXISTS session (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            start_time TIMESTAMP,
            end_time TIMESTAMP,
            status TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(sessionTable)) {
        return false;
    }

    // 创建核心记忆表
    std::string coreMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS core_memory_block (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            key TEXT NOT NULL,
            value TEXT NOT NULL,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id)
        );
    )";
    
    if (!storage.executeNonQuery(coreMemoryTable)) {
        return false;
    }

    // 创建情景记忆表
    std::string episodicMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS episodic_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            session_id INTEGER,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            event TEXT NOT NULL,
            content TEXT NOT NULL,
            event_type TEXT,
            path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";
    
    if (!storage.executeNonQuery(episodicMemoryTable)) {
        return false;
    }

    // 创建语义记忆表
    std::string semanticMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS semantic_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            description TEXT,
            path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(semanticMemoryTable)) {
        return false;
    }

    // 创建程序性记忆表
    std::string proceduralMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS procedural_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            title TEXT NOT NULL,
            description TEXT NOT NULL,
            steps TEXT,
            path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(proceduralMemoryTable)) {
        return false;
    }

    // 创建资源记忆表
    std::string resourceMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS resource_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            resource_type TEXT,
            path TEXT,
            metadata TEXT,
            embedding TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(resourceMemoryTable)) {
        return false;
    }

    // 创建知识库表
    std::string knowledgeVaultTable = R"(
        CREATE TABLE IF NOT EXISTS knowledge_vault (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            title TEXT NOT NULL,
            content TEXT NOT NULL,
            category TEXT,
            path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(knowledgeVaultTable)) {
        return false;
    }

    // 创建归档记忆表
    std::string archivalMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS archival_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            memory_type TEXT NOT NULL,
            content TEXT NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";
    
    if (!storage.executeNonQuery(archivalMemoryTable)) {
        return false;
    }

    // 创建回忆记忆表
    std::string recallMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS recall_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            message_type TEXT NOT NULL,
            content TEXT NOT NULL,
            metadata TEXT,
            embedding TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";
    
    if (!storage.executeNonQuery(recallMemoryTable)) {
        return false;
    }

    // 创建审计日志表
    std::string auditLogTable = R"(
        CREATE TABLE IF NOT EXISTS audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            action TEXT NOT NULL,
            details TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";
    
    if (!storage.executeNonQuery(auditLogTable)) {
        return false;
    }

    // 创建备份表
    std::string backupTable = R"(
        CREATE TABLE IF NOT EXISTS backup (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            data TEXT NOT NULL,
            backup_type TEXT NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id)
        );
    )";
    
    if (!storage.executeNonQuery(backupTable)) {
        return false;
    }

    // 创建多模态数据表
    std::string multimodalDataTable = R"(
        CREATE TABLE IF NOT EXISTS multimodal_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            data_type TEXT NOT NULL,
            file_path TEXT NOT NULL,
            description TEXT,
            emotions TEXT,
            transcription TEXT,
            embedding TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            metadata TEXT
        );
    )";
    
    if (!storage.executeNonQuery(multimodalDataTable)) {
        return false;
    }

    // 创建向量索引表
    std::string embeddingIndexTable = R"(
        CREATE TABLE IF NOT EXISTS embedding_index (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            memory_id INTEGER,
            memory_type TEXT,
            embedding TEXT,
            metadata TEXT
        );
    )";
    
    if (!storage.executeNonQuery(embeddingIndexTable)) {
        return false;
    }

    // 移除外键约束，因为我们可能需要引用不同类型的memory表
    // 外键约束会限制我们只能引用一个特定的表
    
    return true;
}

bool InitDB::createTables(const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database for table creation" << std::endl;
        return false;
    }

    // 启用外键约束
    std::string enableForeignKeys = "PRAGMA foreign_keys = ON;";
    if (!storage.executeQuery(enableForeignKeys)) {
        std::cerr << "Failed to enable foreign keys" << std::endl;
        return false;
    }

    // 创建user表
    std::string createUserTable = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT,
            password_hash TEXT,
            status TEXT DEFAULT 'active',
            last_login_at TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";

    if (!storage.executeNonQuery(createUserTable)) {
        std::cerr << "Failed to create user table" << std::endl;
        return false;
    }

    // 创建agent表
    std::string createAgentTable = R"(
        CREATE TABLE IF NOT EXISTS agent (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT,
            status TEXT,
            description TEXT,
            config TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";

    if (!storage.executeNonQuery(createAgentTable)) {
        std::cerr << "Failed to create agent table" << std::endl;
        return false;
    }

    // 创建session表
    std::string createSessionTable = R"(
        CREATE TABLE IF NOT EXISTS session (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            token TEXT,
            start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            end_time TIMESTAMP,
            status TEXT DEFAULT 'active',
            device_info TEXT,
            metadata TEXT,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id)
        );
    )";

    if (!storage.executeNonQuery(createSessionTable)) {
        std::cerr << "Failed to create session table" << std::endl;
        return false;
    }

    // 创建recall_memory表
    std::string createRecallMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS recall_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            message_type TEXT NOT NULL,
            content TEXT NOT NULL,
            metadata TEXT,
            embedding TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createRecallMemoryTable)) {
        std::cerr << "Failed to create recall_memory table" << std::endl;
        return false;
    }

    // 创建core_memory_block表
    std::string createCoreMemoryBlockTable = R"(
        CREATE TABLE IF NOT EXISTS core_memory_block (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            label TEXT NOT NULL UNIQUE,
            content TEXT NOT NULL,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createCoreMemoryBlockTable)) {
        std::cerr << "Failed to create core_memory_block table" << std::endl;
        return false;
    }

    // 创建episodic_memory表
    std::string createEpisodicMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS episodic_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            timestamp TEXT NOT NULL,
            event TEXT NOT NULL,
            details TEXT,
            event_type TEXT,
            tree_path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createEpisodicMemoryTable)) {
        std::cerr << "Failed to create episodic_memory table" << std::endl;
        return false;
    }

    // 创建semantic_memory表
    std::string createSemanticMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS semantic_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            name TEXT NOT NULL,
            summary TEXT,
            details TEXT,
            tree_path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createSemanticMemoryTable)) {
        std::cerr << "Failed to create semantic_memory table" << std::endl;
        return false;
    }

    // 创建procedural_memory表
    std::string createProceduralMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS procedural_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            title TEXT NOT NULL,
            summary TEXT,
            steps TEXT,
            tree_path TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createProceduralMemoryTable)) {
        std::cerr << "Failed to create procedural_memory table" << std::endl;
        return false;
    }

    // 创建resource_memory表
    std::string createResourceMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS resource_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            filename TEXT NOT NULL,
            resource_type TEXT NOT NULL,
            summary TEXT,
            content TEXT,
            size INTEGER,
            tree_path TEXT,
            metadata TEXT,
            embedding TEXT,
            last_accessed TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createResourceMemoryTable)) {
        std::cerr << "Failed to create resource_memory table" << std::endl;
        return false;
    }

    // 创建knowledge_vault表
    std::string createKnowledgeVaultTable = R"(
        CREATE TABLE IF NOT EXISTS knowledge_vault (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            caption TEXT NOT NULL,
            entry_type TEXT NOT NULL,
            source TEXT,
            sensitivity TEXT,
            encrypted_value TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createKnowledgeVaultTable)) {
        std::cerr << "Failed to create knowledge_vault table" << std::endl;
        return false;
    }

    // 创建archival_memory表
    std::string createArchivalMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS archival_memory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            memory_type TEXT NOT NULL,
            content TEXT NOT NULL,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createArchivalMemoryTable)) {
        std::cerr << "Failed to create archival_memory table" << std::endl;
        return false;
    }

    // 创建embedding_index表
    std::string createEmbeddingIndexTable = R"(
        CREATE TABLE IF NOT EXISTS embedding_index (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            memory_id INTEGER NOT NULL,
            memory_type TEXT NOT NULL,
            embedding TEXT NOT NULL,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (memory_id) REFERENCES episodic_memory(id)
        );
    )";

    if (!storage.executeNonQuery(createEmbeddingIndexTable)) {
        std::cerr << "Failed to create embedding_index table" << std::endl;
        return false;
    }

    // 创建audit_log表
    std::string createAuditLogTable = R"(
        CREATE TABLE IF NOT EXISTS audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            agent_id INTEGER,
            session_id INTEGER,
            action_type TEXT NOT NULL,
            description TEXT,
            details TEXT,
            ip_address TEXT,
            user_agent TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES user(id),
            FOREIGN KEY (agent_id) REFERENCES agent(id),
            FOREIGN KEY (session_id) REFERENCES session(id)
        );
    )";

    if (!storage.executeNonQuery(createAuditLogTable)) {
        std::cerr << "Failed to create audit_log table" << std::endl;
        return false;
    }

    // 创建索引

    return true;
}
