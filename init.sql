CREATE TABLE user (
    id INTEGER PRIMARY KEY AUTOINCREMENT, 
    name TEXT NOT NULL, 
    email TEXT, 
    password_hash TEXT, 
    status TEXT DEFAULT 'active', 
    last_login_at TIMESTAMP, 
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE agent (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    status TEXT DEFAULT 'active',
    config TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id)
);

CREATE TABLE core_memory_block (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    label TEXT NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE episodic_memory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    timestamp TEXT NOT NULL,
    event TEXT NOT NULL,
    details TEXT,
    event_type TEXT,
    tree_path TEXT,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE semantic_memory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    name TEXT NOT NULL,
    summary TEXT NOT NULL,
    details TEXT,
    tree_path TEXT,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE procedural_memory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    title TEXT NOT NULL,
    summary TEXT NOT NULL,
    steps TEXT,
    tree_path TEXT,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE resource_memory (
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

CREATE TABLE knowledge_vault (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    content TEXT NOT NULL,
    content_type TEXT,
    tree_path TEXT,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE archival_memory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER,
    memory_type TEXT NOT NULL,
    content TEXT NOT NULL,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE recall_memory (
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
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

CREATE TABLE embedding_index (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    memory_id INTEGER NOT NULL,
    memory_type TEXT NOT NULL,
    embedding TEXT NOT NULL,
    metadata TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE system_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    agent_id INTEGER,
    action TEXT NOT NULL,
    target_table TEXT,
    target_id INTEGER,
    details TEXT,
    ip_address TEXT,
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user(id),
    FOREIGN KEY (agent_id) REFERENCES agent(id)
);

-- 创建索引以提高查询性能
CREATE INDEX idx_resource_memory_user_id ON resource_memory(user_id);
CREATE INDEX idx_resource_memory_agent_id ON resource_memory(agent_id);
CREATE INDEX idx_resource_memory_resource_type ON resource_memory(resource_type);

CREATE INDEX idx_recall_memory_user_id ON recall_memory(user_id);
CREATE INDEX idx_recall_memory_agent_id ON recall_memory(agent_id);
CREATE INDEX idx_recall_memory_session_id ON recall_memory(session_id);
CREATE INDEX idx_recall_memory_created_at ON recall_memory(created_at);

CREATE INDEX idx_episodic_memory_user_id ON episodic_memory(user_id);
CREATE INDEX idx_episodic_memory_agent_id ON episodic_memory(agent_id);
CREATE INDEX idx_episodic_memory_timestamp ON episodic_memory(timestamp);

CREATE INDEX idx_semantic_memory_user_id ON semantic_memory(user_id);
CREATE INDEX idx_semantic_memory_agent_id ON semantic_memory(agent_id);
CREATE INDEX idx_semantic_memory_name ON semantic_memory(name);

CREATE INDEX idx_procedural_memory_user_id ON procedural_memory(user_id);
CREATE INDEX idx_procedural_memory_agent_id ON procedural_memory(agent_id);
CREATE INDEX idx_procedural_memory_title ON procedural_memory(title);

CREATE INDEX idx_core_memory_block_user_id ON core_memory_block(user_id);
CREATE INDEX idx_core_memory_block_agent_id ON core_memory_block(agent_id);
CREATE INDEX idx_core_memory_block_label ON core_memory_block(label);

CREATE INDEX idx_knowledge_vault_user_id ON knowledge_vault(user_id);
CREATE INDEX idx_knowledge_vault_agent_id ON knowledge_vault(agent_id);

CREATE INDEX idx_archival_memory_user_id ON archival_memory(user_id);
CREATE INDEX idx_archival_memory_agent_id ON archival_memory(agent_id);

CREATE INDEX idx_agent_user_id ON agent(user_id);

CREATE INDEX idx_system_log_user_id ON system_log(user_id);
CREATE INDEX idx_system_log_agent_id ON system_log(agent_id);
CREATE INDEX idx_system_log_created_at ON system_log(created_at);