# 记忆系统文档

## 概述

MIRIX系统实现了仿人类的完整记忆系统，包括8种不同类型的记忆。每种记忆类型都有专门的数据表和管理器，用于存储和管理不同类型的信息。

所有记忆数据统一存储在项目根目录下的[database](../src/database)目录中，确保数据持久化且不会因构建目录清理而丢失。

## 记忆类型详解

### 1. 回忆记忆 (Recall Memory)
存储对话历史和记忆检索过程，是系统中最活跃的记忆类型。

**用途**：
- 存储用户与AI的对话历史
- 记录记忆检索过程和结果
- 提供上下文信息用于对话生成

**数据表**：recall_memory
**管理器**：RecallMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- session_id：会话ID
- message_type：消息类型(user_message, ai_response)
- content：消息内容
- embedding：消息内容的向量表示

### 2. 资源记忆 (Resource Memory)
存储多媒体资源文件的元信息，包括图像、音频、屏幕截图等。

**用途**：
- 存储图像、音频等多媒体资源的元信息
- 为多模态处理提供资源索引
- 支持跨模态搜索和检索

**数据表**：resource_memory
**管理器**：ResourceMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- session_id：会话ID
- path：资源文件路径
- resource_type：资源类型(image, audio, screenshot)
- description：资源描述
- embedding：资源描述的向量表示

### 3. 核心记忆 (Core Memory)
存储最基本和重要的信息块。

**用途**：
- 存储用户的核心个人信息
- 存储系统配置信息
- 提供快速访问的关键信息

**数据表**：core_memory_block
**管理器**：CoreMemoryBlockManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- label：信息块标签
- value：信息块值

### 4. 事件记忆 (Episodic Memory)
存储个人经历和事件相关信息。

**用途**：
- 存储用户的个人经历
- 记录重要事件和时间点
- 支持基于时间的回忆和检索

**数据表**：episodic_memory
**管理器**：EpisodicMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- timestamp：事件时间戳
- summary：事件摘要
- details：事件详细信息
- event_type：事件类型

### 5. 语义记忆 (Semantic Memory)
存储概念性知识和事实信息。

**用途**：
- 存储概念性知识
- 存储事实信息
- 支持知识推理和检索

**数据表**：semantic_memory
**管理器**：SemanticMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- name：知识名称
- summary：知识摘要
- details：知识详细信息

### 6. 程序记忆 (Procedural Memory)
存储技能和过程性知识。

**用途**：
- 存储技能信息
- 存储过程性知识
- 支持技能学习和执行

**数据表**：procedural_memory
**管理器**：ProceduralMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- title：技能标题
- summary：技能摘要
- steps：执行步骤

### 7. 知识库 (Knowledge Vault)
存储结构化知识和信息。

**用途**：
- 存储结构化知识
- 存储敏感信息（加密存储）
- 支持知识分类和检索

**数据表**：knowledge_vault
**管理器**：KnowledgeVaultManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- caption：知识标题
- entry_type：条目类型
- source：来源
- sensitivity：敏感性级别
- encrypted_value：加密值

### 8. 归档记忆 (Archival Memory)
存储长期保存的历史数据。

**用途**：
- 存储历史数据
- 支持长期数据保存
- 减轻活跃记忆系统的负担

**数据表**：archival_memory
**管理器**：ArchivalMemoryManager
**主要字段**：
- user_id：用户ID
- agent_id：代理ID
- content：归档内容
- archive_date：归档日期

## 记忆管理器

每种记忆类型都有对应的管理器类，负责对该类型记忆的增删改查操作。所有管理器都通过StorageManager与数据库进行交互，确保数据的一致性和完整性。

## 向量表示

为了支持向量检索功能，系统为回忆记忆和资源记忆添加了向量表示(embedding字段)。这些向量通过以下方式生成：

1. **文本向量**：使用TextEmbeddingGenerator生成
2. **图像向量**：通过CLIPProcessor生成图像描述，再基于描述生成向量

所有向量都统一为1024维，并通过VectorSerializer进行序列化存储。

## 记忆检索

系统支持多种记忆检索方式：

1. **基于内容的检索**：直接查询记忆内容
2. **基于向量的检索**：使用向量相似性搜索
3. **基于时间的检索**：根据时间范围检索记忆
4. **跨模态检索**：支持文本到图像、图像到文本的检索

## 记忆持久化

所有记忆数据都持久化存储在SQLite数据库中，确保系统重启后数据不会丢失。数据库文件统一存储在项目根目录下的[database](../src/database)目录中。