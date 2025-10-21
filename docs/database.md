# MIRIX数据库设计文档

## 1. 概述

MIRIX系统使用SQLite作为后端数据库，采用关系型数据模型设计。数据库设计遵循人类记忆系统的结构，包含多个专门用于存储不同类型记忆的表。

## 2. 数据库表结构

### 2.1 核心表

#### 2.1.1 user表 - 用户信息表
存储系统用户的基本信息。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 用户唯一标识符
- name (TEXT NOT NULL): 用户名
- email (TEXT): 用户邮箱
- password_hash (TEXT): 密码哈希值
- status (TEXT DEFAULT 'active'): 用户状态
- last_login_at (TIMESTAMP): 最后登录时间
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间

#### 2.1.2 agent表 - 代理信息表
存储系统代理的基本信息。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 代理唯一标识符
- name (TEXT NOT NULL): 代理名
- description (TEXT): 代理描述
- capabilities (TEXT): 代理能力
- status (TEXT DEFAULT 'active'): 代理状态
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间

#### 2.1.3 session表 - 会话信息表
存储用户会话信息。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 会话唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- start_time (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 会话开始时间
- end_time (TIMESTAMP): 会话结束时间
- status (TEXT DEFAULT 'active'): 会话状态
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

### 2.2 记忆系统表

#### 2.2.1 resource_memory表 - 资源记忆表
存储多模态资源数据（图像、音频、屏幕截图等）。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 资源唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- session_id (INTEGER): 关联的会话ID
- filename (TEXT NOT NULL): 文件路径和名称
- resource_type (TEXT NOT NULL): 资源类型（image/audio/screenshot）
- summary (TEXT): 资源摘要
- content (TEXT): 资源内容描述
- size (INTEGER): 文件大小
- tree_path (TEXT): 树形路径
- metadata (TEXT): 元数据（JSON格式）
- last_accessed (TIMESTAMP): 最后访问时间
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- updated_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 更新时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)
- FOREIGN KEY (session_id) REFERENCES session(id)

#### 2.2.2 core_memory_block表 - 核心记忆块表
存储核心记忆数据块。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 记忆块唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- content (TEXT NOT NULL): 记忆内容
- importance (INTEGER DEFAULT 5): 重要性等级(1-10)
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.3 episodic_memory表 - 事件记忆表
存储个人经历和事件记忆。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 事件记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- content (TEXT NOT NULL): 事件内容
- summary (TEXT): 事件摘要
- start_time (TIMESTAMP): 事件开始时间
- end_time (TIMESTAMP): 事件结束时间
- location (TEXT): 事件地点
- participants (TEXT): 参与者
- emotions (TEXT): 情感状态
- importance (INTEGER DEFAULT 5): 重要性等级(1-10)
- tags (TEXT): 标签
- metadata (TEXT): 元数据
- embedding (BLOB): 向量嵌入
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.4 semantic_memory表 - 语义记忆表
存储概念性知识和事实。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 语义记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- concept (TEXT NOT NULL): 概念名称
- definition (TEXT): 概念定义
- category (TEXT): 分类
- relationships (TEXT): 关系
- facts (TEXT): 事实
- importance (INTEGER DEFAULT 5): 重要性等级(1-10)
- tags (TEXT): 标签
- metadata (TEXT): 元数据
- embedding (BLOB): 向量嵌入
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.5 procedural_memory表 - 程序记忆表
存储技能和过程性知识。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 程序记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- skill_name (TEXT NOT NULL): 技能名称
- description (TEXT): 技能描述
- steps (TEXT): 执行步骤
- prerequisites (TEXT): 先决条件
- difficulty (INTEGER): 难度等级(1-10)
- proficiency (INTEGER DEFAULT 1): 熟练程度(1-10)
- tags (TEXT): 标签
- metadata (TEXT): 元数据
- embedding (BLOB): 向量嵌入
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.6 knowledge_vault表 - 知识库表
存储结构化知识。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 知识条目唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- title (TEXT NOT NULL): 知识标题
- content (TEXT): 知识内容
- category (TEXT): 分类
- keywords (TEXT): 关键词
- source (TEXT): 来源
- reliability (INTEGER DEFAULT 5): 可靠性等级(1-10)
- tags (TEXT): 标签
- metadata (TEXT): 元数据
- embedding (BLOB): 向量嵌入
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.7 archival_memory表 - 归档记忆表
存储长期归档数据。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 归档记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- content (TEXT NOT NULL): 归档内容
- summary (TEXT): 内容摘要
- archive_date (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 归档日期
- retention_policy (TEXT): 保留策略
- tags (TEXT): 标签
- metadata (TEXT): 元数据
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- accessed_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 访问时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.8 recall_memory表 - 回忆记忆表
存储回忆过程和记忆检索。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 回忆记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- query (TEXT NOT NULL): 查询内容
- results (TEXT): 检索结果
- relevance_scores (TEXT): 相关性评分
- recall_time (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 回忆时间
- duration (INTEGER): 检索耗时(毫秒)
- success (INTEGER DEFAULT 1): 是否成功(0/1)
- metadata (TEXT): 元数据
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

### 2.3 系统表

#### 2.3.1 audit_log表 - 审计日志表
记录系统操作日志。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 日志唯一标识符
- user_id (INTEGER): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- action (TEXT NOT NULL): 操作类型
- target_table (TEXT): 目标表
- target_id (INTEGER): 目标ID
- details (TEXT): 详细信息
- ip_address (TEXT): IP地址
- user_agent (TEXT): 用户代理
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

## 3. 索引设计

为提高查询性能，系统在以下字段上创建了索引：

1. resource_memory表：
   - user_id字段索引
   - agent_id字段索引
   - resource_type字段索引

2. 所有记忆表：
   - user_id字段索引
   - agent_id字段索引
   - created_at字段索引

## 4. 数据库管理

### 4.1 存储管理器 (StorageManager)
封装了所有数据库操作，提供以下核心方法：
- executeQuery(): 执行查询语句
- executeNonQuery(): 执行非查询语句
- getDatabase(): 获取数据库连接

### 4.2 事务管理器 (TransactionManager)
管理数据库事务，提供以下方法：
- beginTransaction(): 开始事务
- commitTransaction(): 提交事务
- rollbackTransaction(): 回滚事务

## 5. 数据一致性

系统通过以下机制保证数据一致性：
1. 外键约束：确保表间关联数据的一致性
2. 事务管理：确保复杂操作的原子性
3. 时间戳字段：跟踪数据创建和更新时间
4. 数据验证：在应用层进行数据有效性检查