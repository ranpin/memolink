# 数据库设计文档

## 概述

MIRIX系统使用SQLite作为主要的数据存储解决方案，实现了完整的数据库设计来支持仿人类记忆系统的各种功能。系统包含多个表来存储不同类型的记忆信息，并通过专门的管理器类来操作这些数据。

所有数据库文件统一存储在项目根目录下的[database](file:///Users/ranpin/code/mirix-cpp/src/database)目录中，避免了在build目录中创建数据库文件的问题。

## 数据库表结构

### 1. 用户表 (user)
存储系统用户信息：
- id: INTEGER PRIMARY KEY
- name: TEXT
- email: TEXT
- password_hash: TEXT
- created_at: TEXT

### 2. 会话表 (session)
存储用户会话信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- token: TEXT
- start_time: TEXT
- end_time: TEXT
- status: TEXT
- device_info: TEXT
- metadata: TEXT

### 3. Agent表 (agent)
存储AI代理信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- name: TEXT
- description: TEXT
- capabilities: TEXT
- created_at: TEXT

### 4. 回忆记忆表 (message_memory)
存储对话历史和记忆检索过程：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- session_id: INTEGER
- message_type: TEXT
- content: TEXT
- metadata: TEXT
- embedding: BLOB
- created_at: TEXT
- updated_at: TEXT

### 5. 资源记忆表 (resource_memory)
存储多媒体资源文件的元信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- session_id: INTEGER
- path: TEXT
- resource_type: TEXT
- description: TEXT
- size: INTEGER
- metadata: TEXT
- embedding: BLOB
- created_at: TEXT
- updated_at: TEXT

### 6. 核心记忆块表 (core_memory_block)
存储核心记忆信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- label: TEXT
- value: TEXT
- created_at: TEXT
- updated_at: TEXT

### 7. 事件记忆表 (episodic_memory)
存储事件记忆信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- timestamp: TEXT
- summary: TEXT
- details: TEXT
- event_type: TEXT
- metadata: TEXT
- created_at: TEXT
- updated_at: TEXT

### 8. 语义记忆表 (semantic_memory)
存储语义记忆信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- name: TEXT
- summary: TEXT
- details: TEXT
- metadata: TEXT
- created_at: TEXT
- updated_at: TEXT

### 9. 程序记忆表 (procedural_memory)
存储程序记忆信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- title: TEXT
- summary: TEXT
- steps: TEXT
- metadata: TEXT
- created_at: TEXT
- updated_at: TEXT

### 10. 私密信息库表 (secret_vault)
存储私密信息库信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- caption: TEXT
- entry_type: TEXT
- source: TEXT
- sensitivity: TEXT
- encrypted_value: TEXT
- metadata: TEXT
- created_at: TEXT
- updated_at: TEXT

### 11. 归档记忆表 (archival_memory)
存储归档记忆信息：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- content: TEXT
- archive_date: TEXT
- metadata: TEXT
- created_at: TEXT
- updated_at: TEXT

### 12. 审计日志表 (audit_log)
存储系统操作日志：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- action_type: TEXT
- memory_id: INTEGER
- memory_type: TEXT
- timestamp: TEXT
- metadata: TEXT
- session_id: INTEGER

### 13. 备份表 (backup)
存储备份信息：
- id: INTEGER PRIMARY KEY
- backup_name: TEXT
- backup_path: TEXT
- backup_date: TEXT
- status: TEXT
- metadata: TEXT

### 14. 可视化数据表 (visualization_data)
存储可视化相关数据：
- id: INTEGER PRIMARY KEY
- user_id: INTEGER
- agent_id: INTEGER
- data_type: TEXT
- content: TEXT
- metadata: TEXT
- created_at: TEXT

## 数据库管理器

系统为每个数据表提供了专门的管理器类：

- **UserManager**：管理用户信息
- **SessionManager**：管理会话信息
- **MessageMemoryManager**：管理回忆记忆
- **ResourceMemoryManager**：管理资源记忆
- **CoreMemoryBlockManager**：管理核心记忆块
- **EpisodicMemoryManager**：管理事件记忆
- **SemanticMemoryManager**：管理语义记忆
- **ProceduralMemoryManager**：管理程序记忆
- **SecretVaultManager**：管理私密信息库
- **ArchivalMemoryManager**：管理归档记忆
- **AuditLogManager**：管理审计日志
- **BackupManager**：管理备份
- **VisualizationManager**：管理可视化数据

## 数据库初始化

系统通过InitDB类来初始化数据库，包括创建表结构和索引。数据库文件统一存储在项目根目录下的[database](file:///Users/ranpin/code/mirix-cpp/src/database)目录中，确保数据持久化且不会因构建目录清理而丢失。

## 存储管理

系统使用StorageManager类来管理数据库连接和事务处理，确保数据操作的原子性和一致性。

## 向量数据存储

为了支持向量检索功能，系统在需要的表中添加了embedding字段（BLOB类型），用于存储向量数据。这些向量数据通过TextEmbeddingGenerator和CLIPProcessor生成，并通过VectorSerializer进行序列化和反序列化。

## 测试数据库

为了避免测试数据污染主系统数据库，每个测试程序都会创建独立的测试数据库，存储在[database](file:///Users/ranpin/code/mirix-cpp/src/database)目录中，文件名根据测试程序命名，例如：
- [test_chat_with_model.db](file:///Users/ranpin/code/mirix-cpp/database/test_chat_with_model.db)
- [test_embedding.db](file:///Users/ranpin/code/mirix-cpp/database/test_embedding.db)
- [system_initialization_test.db](file:///Users/ranpin/code/mirix-cpp/database/system_initialization_test.db)