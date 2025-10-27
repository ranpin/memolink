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

#### 2.2.4 recall_memory表 - 回忆记忆表
存储对话历史和记忆检索过程。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 回忆记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- session_id (INTEGER): 关联的会话ID
- message_type (TEXT NOT NULL): 消息类型（user_message/ai_response）
- content (TEXT NOT NULL): 消息内容
- metadata (TEXT): 元数据
- embedding (BLOB): 向量表示（CLIP生成的512维向量）
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- updated_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 更新时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)
- FOREIGN KEY (session_id) REFERENCES session(id)

#### 2.2.5 semantic_memory表 - 语义记忆表
存储概念性知识和事实信息。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 语义记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- title (TEXT NOT NULL): 标题
- content (TEXT NOT NULL): 内容
- description (TEXT): 描述
- path (TEXT): 路径
- metadata (TEXT): 元数据
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.6 procedural_memory表 - 程序记忆表
存储技能和过程性知识。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 程序记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- title (TEXT NOT NULL): 标题
- description (TEXT NOT NULL): 描述
- steps (TEXT): 步骤
- path (TEXT): 路径
- metadata (TEXT): 元数据
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.7 knowledge_vault表 - 知识库表
存储结构化知识和信息。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 知识库条目唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- title (TEXT NOT NULL): 标题
- content (TEXT NOT NULL): 内容
- category (TEXT): 分类
- path (TEXT): 路径
- metadata (TEXT): 元数据
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

#### 2.2.8 archival_memory表 - 归档记忆表
存储长期保存的历史数据。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 归档记忆唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- memory_type (TEXT NOT NULL): 记忆类型
- content (TEXT NOT NULL): 内容
- timestamp (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 时间戳
- metadata (TEXT): 元数据
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)

### 2.3 系统表

#### 2.3.1 audit_log表 - 审计日志表
存储系统操作日志。

字段：
- id (INTEGER PRIMARY KEY AUTOINCREMENT): 日志唯一标识符
- user_id (INTEGER NOT NULL): 关联的用户ID
- agent_id (INTEGER): 关联的代理ID
- session_id (INTEGER): 关联的会话ID
- action_type (TEXT NOT NULL): 操作类型
- description (TEXT): 描述
- details (TEXT): 详细信息
- ip_address (TEXT): IP地址
- user_agent (TEXT): 用户代理
- created_at (TIMESTAMP DEFAULT CURRENT_TIMESTAMP): 创建时间
- FOREIGN KEY (user_id) REFERENCES user(id)
- FOREIGN KEY (agent_id) REFERENCES agent(id)
- FOREIGN KEY (session_id) REFERENCES session(id)

## 3. 索引设计

为了提高查询性能，系统在关键字段上建立了索引：

### 3.1 用户相关索引
- idx_resource_memory_user_id: resource_memory表的user_id字段
- idx_recall_memory_user_id: recall_memory表的user_id字段
- idx_episodic_memory_user_id: episodic_memory表的user_id字段
- idx_semantic_memory_user_id: semantic_memory表的user_id字段
- idx_procedural_memory_user_id: procedural_memory表的user_id字段

### 3.2 时间相关索引
- idx_recall_memory_created_at: recall_memory表的created_at字段
- idx_resource_memory_created_at: resource_memory表的created_at字段

### 3.3 类型相关索引
- idx_resource_memory_resource_type: resource_memory表的resource_type字段
- idx_recall_memory_message_type: recall_memory表的message_type字段

### 3.4 会话相关索引
- idx_recall_memory_session_id: recall_memory表的session_id字段

## 4. 多模态数据存储策略

### 4.1 数据分离存储
- 结构化数据存储在SQLite数据库中
- 大型文件（图像、音频等）存储在文件系统中
- 数据库中保存文件路径和元数据信息

### 4.2 文件组织结构
```
captures/
├── camera/          # 摄像头图像
├── audio/           # 音频文件
└── screen/          # 屏幕截图
```

### 4.3 元数据管理
- 文件基本信息（大小、创建时间等）
- 内容描述和摘要
- 标签和分类信息
- 处理状态和版本信息

## 5. 跨会话查询支持

### 5.1 会话管理机制
- 每次用户交互创建新的会话记录
- 会话通过session_id与各种记忆数据关联
- 会话状态管理（active/ended）

### 5.2 跨会话查询实现
- 通过user_id关联用户的所有会话
- 支持查询用户最近N次会话的历史记录
- 可以按时间范围筛选会话

### 5.3 查询示例

#### 5.3.1 获取用户最近的会话
```sql
SELECT id FROM session 
WHERE user_id = ? 
ORDER BY id DESC 
LIMIT ?;
```

#### 5.3.2 查询多个会话中的回忆记忆
```sql
SELECT * FROM recall_memory 
WHERE user_id = ? AND session_id IN (?, ?, ?) 
ORDER BY created_at DESC 
LIMIT ?;
```

#### 5.3.3 查询用户所有回忆记忆
```sql
SELECT * FROM recall_memory 
WHERE user_id = ? 
ORDER BY created_at DESC 
LIMIT ?;
```

## 6. 向量化数据存储

### 6.1 向量字段设计
- 使用BLOB类型存储二进制向量数据
- 支持CLIP模型生成的512维向量
- 在recall_memory和resource_memory表中都有embedding字段

### 6.2 向量序列化
- 向量数据以二进制格式序列化存储
- 支持float32类型的向量数据
- 提供序列化和反序列化接口

### 6.3 向量索引
- 考虑集成FAISS等向量搜索引擎
- 支持近似最近邻搜索
- 提高大规模向量数据的检索效率

## 7. 数据一致性与事务管理

### 7.1 事务设计
- 使用SQLite事务确保数据一致性
- 支持批量操作的原子性
- 提供错误回滚机制

### 7.2 外键约束
- 建立表间外键关系
- 确保数据引用完整性
- 支持级联操作

### 7.3 数据验证
- 在应用层进行数据验证
- 确保数据格式和范围正确
- 提供错误处理机制