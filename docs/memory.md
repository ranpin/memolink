# MIRIX记忆系统设计文档

## 1. 概述

MIRIX记忆系统基于人类记忆模型设计，包含多种类型的记忆存储，用于存储和管理不同类型的信息。系统通过模拟人类记忆的工作方式，实现信息的存储、检索和管理。

## 2. 记忆系统架构

### 2.1 记忆类型

#### 2.1.1 核心记忆 (Core Memory)
存储最基本和重要的信息块，是系统运行的核心数据。

**管理器**: CoreMemoryBlockManager
**存储表**: core_memory_block

**特征**:
- 高优先级存储
- 不易丢失
- 快速访问

#### 2.1.2 事件记忆 (Episodic Memory)
存储个人经历和事件相关信息，包含时间、地点、参与者等上下文信息。

**管理器**: EpisodicMemoryManager
**存储表**: episodic_memory

**特征**:
- 时间序列化存储
- 包含丰富上下文
- 支持情感标记

#### 2.1.3 语义记忆 (Semantic Memory)
存储概念性知识和事实信息，如词汇、概念、规则等。

**管理器**: SemanticMemoryManager
**存储表**: semantic_memory

**特征**:
- 结构化知识存储
- 概念关联网络
- 分类管理

#### 2.1.4 程序记忆 (Procedural Memory)
存储技能和过程性知识，如如何执行特定任务。

**管理器**: ProceduralMemoryManager
**存储表**: procedural_memory

**特征**:
- 步骤化存储
- 熟练度跟踪
- 条件触发

#### 2.1.5 资源记忆 (Resource Memory)
存储多媒体资源文件的元信息，包括图像、音频、视频等。

**管理器**: ResourceMemoryManager
**存储表**: resource_memory

**特征**:
- 文件元信息管理
- 多媒体类型支持
- 路径索引
- 向量化检索支持

#### 2.1.6 知识库 (Knowledge Vault)
存储结构化知识和信息，如文档、文章等。

**管理器**: KnowledgeVaultManager
**存储表**: knowledge_vault

**特征**:
- 结构化存储
- 来源追踪
- 可靠性评估

#### 2.1.7 归档记忆 (Archival Memory)
存储长期保存的历史数据，用于备份和历史查询。

**管理器**: ArchivalMemoryManager
**存储表**: archival_memory

**特征**:
- 长期存储
- 压缩优化
- 策略管理

#### 2.1.8 回忆记忆 (Recall Memory)
存储记忆检索过程和结果，用于优化检索算法。

**管理器**: RecallMemoryManager
**存储表**: recall_memory

**特征**:
- 检索过程记录
- 相关性评分
- 性能监控
- 对话历史存储
- 跨会话历史查询支持

## 3. 记忆管理器设计

### 3.1 通用接口

所有记忆管理器都遵循统一的设计模式，提供以下通用接口：

```cpp
class MemoryManager {
public:
    // 添加记忆
    virtual bool add_memory(const MemoryItem& item) = 0;
    
    // 更新记忆
    virtual bool update_memory(int id, const MemoryItem& item) = 0;
    
    // 删除记忆
    virtual bool delete_memory(int id) = 0;
    
    // 查询记忆
    virtual std::vector<MemoryItem> search_memories(const SearchCriteria& criteria) = 0;
    
    // 向量搜索记忆
    virtual std::vector<MemoryItem> vector_search(const std::vector<float>& query_vector, int limit) = 0;
};
```

### 3.2 多模态数据支持

#### 3.2.1 文本数据处理
- 文本内容直接存储在content字段中
- 使用CLIP模型生成1024维向量表示
- 向量存储在embedding字段中（BLOB类型）

#### 3.2.2 图像数据处理
- 图像文件存储在文件系统中
- 文件路径存储在path字段中
- 图像元数据存储在metadata字段中（JSON格式）
- 使用CLIP模型生成1024维向量表示
- 向量存储在embedding字段中（BLOB类型）

#### 3.2.3 音频数据处理
- 音频文件存储在文件系统中
- 文件路径存储在path字段中
- 音频元数据存储在metadata字段中（JSON格式）
- 音频内容摘要存储在summary字段中
- 可选：使用音频处理模型生成向量表示

### 3.3 跨会话历史查询

#### 3.3.1 设计原理
- 每次用户交互创建新的会话记录
- 会话数据通过session_id关联
- 支持查询用户最近N次会话的历史记录
- 保持会话间的数据隔离性

#### 3.3.2 实现机制
- 使用`list_recent_sessions_memories`方法实现跨会话查询
- 先获取用户最近的会话ID列表
- 再查询这些会话中的回忆记忆
- 支持限制返回的消息数量

#### 3.3.3 查询示例
```sql
-- 获取用户最近几次会话的ID
SELECT id FROM session WHERE user_id = ? ORDER BY id DESC LIMIT ?;

-- 查询这些会话中的回忆记忆
SELECT * FROM recall_memory 
WHERE user_id = ? AND session_id IN (?, ?, ?) 
ORDER BY created_at DESC LIMIT ?;
```

## 4. 向量化检索设计

### 4.1 向量生成
- 使用CLIP模型为文本和图像生成1024维向量
- 向量存储在各记忆表的embedding字段中
- 支持实时向量生成和批量处理

### 4.2 向量存储
- 向量以BLOB格式存储在SQLite数据库中
- 支持二进制向量数据的读写操作
- 建立索引优化向量检索性能

### 4.3 相似性搜索
- 支持余弦相似度计算
- 支持欧氏距离计算
- 支持混合排序算法

## 5. 数据持久化策略

### 5.1 数据库设计
- 使用SQLite作为后端存储
- 采用关系型数据模型设计
- 建立适当的索引提高查询性能

### 5.2 事务管理
- 使用事务确保数据一致性
- 支持批量操作的原子性
- 提供错误回滚机制

### 5.3 备份策略
- 定期备份数据库文件
- 支持增量备份和全量备份
- 提供数据恢复机制

## 6. 性能优化

### 6.1 查询优化
- 建立合适的数据库索引
- 优化SQL查询语句
- 使用分页机制处理大量数据

### 6.2 向量检索优化
- 使用FAISS库进行向量索引管理
- 支持近似最近邻搜索
- 优化向量存储和检索性能

### 6.3 内存管理
- 合理使用缓存机制
- 及时释放不需要的资源
- 优化数据结构减少内存占用

## 7. AI模型集成

### 7.1 云端模型支持
- 集成阿里云通义千问API
- 支持文本生成和对话功能
- 需要配置API密钥

### 7.2 本地模型支持
- 支持OpenAI兼容的本地服务
- 通过配置local_url使用本地API
- 优先级高于云端API

### 7.3 模拟模型支持
- 在无API配置时提供模拟响应
- 用于开发和测试环境
- 最低优先级

调用优先级：本地API > 云端API > 模拟响应