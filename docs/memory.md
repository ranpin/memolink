# 记忆系统文档

## 概述

MIRIX系统实现了仿人类的完整记忆系统，当前包括8种不同类型的记忆。每种记忆类型都有专门的数据表和管理器，用于存储和管理不同类型的信息。所有记忆数据统一存储在项目根目录下的 `database/` 目录中，确保数据持久化且不会因构建目录清理而丢失。

## 当前记忆类型详解

### 1. 消息记忆 (Message Memory)

存储用户与代理之间的所有对话消息，是系统中最活跃的记忆类型，同时作为长期记忆生成的主要原材料之一。

**用途**：
- 存储用户与 AI 的对话历史
- 为上下文构建提供依据
- 作为长期记忆生成的输入（未处理消息）
- 记录消息是否已用于长期记忆生成（processed 标记）

**数据表**：`message_memories`  
**管理器**：`MessageMemoryManager`  
**主要字段**（参见 `src/database/models/MemoryModels.h`）：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `session_id`：会话ID
- `content`：消息内容（纯文本）
- `created_at`：消息时间
- `processed`：是否已经用于长期记忆生成（0=未处理，1=已处理）

### 2. 多模态记忆 (Multimodal Memory)

存储图像、音频、屏幕截图等多模态数据的元信息与语义描述。

**用途**：
- 存储图像、音频、屏幕截图等多媒体资源的文件路径
- 保存由多模态大模型生成的语义描述（description）
- 为长期记忆生成提供多模态原材料
- 支持跨模态检索和场景理解

**数据表**：`multimodal_memories`  
**管理器**：通过 `MultimodalDataManager` 操作，底层使用 `ORMWrapper`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `session_id`：会话ID
- `data_type`：数据类型（image/audio/screenshot）
- `file_path`：文件存储路径（相对或绝对路径）
- `description`：**语义描述（优先使用多模态大模型输出）**
- `metadata`：元数据（JSON格式，可包含时长、分辨率等）
- `created_at`：创建时间

**关键设计**：  
- `description` 字段是多模态数据的主语义字段，优先使用多模态大模型（如 qwen2.5-omni）生成的自然语言场景描述。
- 在记忆生成阶段，`description` 会作为文本来源（MemorySource.text），与消息一起参与长期记忆生成。

### 3. 情景记忆 (Episodic Memory)

存储个人经历和事件相关信息。

**用途**：
- 存储用户的个人经历和具体事件
- 记录重要事件和时间点
- 支持基于时间和场景的回忆检索

**数据表**：`episodic_memories`  
**管理器**：`EpisodicMemoryManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `session_id`：会话ID
- `event_description`：事件描述
- `participants`：参与者
- `location`：地点
- `timestamp`：事件时间戳
- `importance`：重要性评分
- `metadata`：附加元数据
- `created_at`：创建时间

### 4. 语义记忆 (Semantic Memory)

存储概念性知识和事实信息。

**用途**：
- 存储事实性、概念性知识
- 存储抽象的知识和规则
- 支持知识推理和检索

**数据表**：`semantic_memories`  
**管理器**：`SemanticMemoryManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `fact`：事实或知识内容
- `category`：知识类别
- `confidence`：置信度（0-1）
- `metadata`：附加元数据
- `created_at`：创建时间

### 5. 核心记忆 (Core Memory)

存储最基本和重要的信息块。

**用途**：
- 存储用户的核心个人信息
- 存储对代理非常重要的长期信息
- 提供快速访问的关键信息

**数据表**：`core_memories`  
**管理器**：`CoreMemoryBlockManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `memory_type`：记忆类型标签
- `content`：内容
- `priority`：优先级
- `metadata`：附加元数据
- `created_at`：创建时间

### 6. 程序记忆 (Procedural Memory)

存储技能和过程性知识。

**用途**：
- 存储技能信息和操作步骤
- 存储习惯性行为模式
- 支持技能学习和执行

**数据表**：`procedural_memories`  
**管理器**：`ProceduralMemoryManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `name`：技能或流程名称
- `description`：描述
- `steps`：执行步骤（可以是 JSON 或文本）
- `metadata`：附加元数据
- `created_at`：创建时间

### 7. 秘密保险库 (Secret Vault)

存储敏感和隐私信息。

**用途**：
- 存储用户的敏感信息
- 存储需要保密的知识
- 支持敏感信息的安全管理

**数据表**：`secret_vault`  
**管理器**：`SecretVaultManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `agent_id`：代理ID
- `category`：类别
- `title`：标题
- `content`：内容
- `metadata`：附加元数据
- `created_at`：创建时间

### 8. 记忆溯源 (Memory Provenance)

记录长期记忆与消息、多模态来源之间的多对多关系，使每条长期记忆都可以反查"由哪些证据生成"。

**用途**：
- 记录长期记忆的生成来源（消息或多模态数据）
- 支持记忆的可解释性和可追溯性
- 为调试和分析记忆生成过程提供依据

**数据表**：`memory_provenance`  
**管理器**：`MemoryProvenanceManager`  
**主要字段**：
- `id`：主键
- `memory_type`：记忆类型（"episodic"/"semantic"/"core"/"procedural"/"secret_vault"）
- `memory_id`：对应记忆的ID
- `source_type`：来源类型（"message"/"multimodal"）
- `source_id`：来源ID（指向 message_memories 或 multimodal_memories）
- `relation_type`：关系类型（如 "evidence"）
- `created_at`：创建时间

**溯源查询示例**：
```cpp
// 查询某条情景记忆的所有来源
auto provenances = provenanceManager.getByMemory("episodic", episodic_memory_id);
for (const auto& p : provenances) {
    if (p.source_type == "message") {
        auto msg = messageManager.getById(p.source_id);
        // 展示消息内容
    } else if (p.source_type == "multimodal") {
        auto mm = orm.getById<MultimodalMemory>(p.source_id);
        // 展示多模态描述和文件路径
    }
}
```

### 9. 会话 (Session)

存储会话的边界和元信息。

**用途**：
- 管理对话会话的生命周期
- 关联同一会话中的消息和记忆
- 支持会话级别的记忆检索

**数据表**：`sessions`  
**管理器**：`SessionManager`  
**主要字段**：
- `id`：主键
- `user_id`：用户ID
- `start_time`：会话开始时间
- `end_time`：会话结束时间（可为空）
- `metadata`：会话元数据

## 记忆生成与演化

### 记忆生成流程

MIRIX 系统通过 `MemoryGenerationManager` 实现从原材料（消息+多模态）到长期记忆的自动生成：

1. **原材料收集**：从 `message_memories`（未处理消息）和 `multimodal_memories` 收集原材料
2. **触发条件判断**：基于配置的数量阈值（`message_threshold`）判断是否触发生成
3. **记忆分类**：调用大模型对消息进行分类，判断应生成哪种类型的长期记忆
4. **记忆生成**：调用相应的 Generator（Episodic/Semantic/Core/Procedural/SecretVault）生成记忆
5. **溯源写入**：为生成的长期记忆写入 `memory_provenance` 记录
6. **标记处理**：将消息标记为已处理（`processed = 1`）

### 配置驱动

记忆生成行为由 `config.ini` 中的 `[memory_generation]` 配置节控制：

```ini
[memory_generation]
message_threshold=3              # 触发生成的原材料数量阈值
confidence_threshold=0.5         # 分类置信度阈值
max_sources_per_generation=50    # 单次生成最多使用的来源数量
enable_provenance=true           # 是否写入溯源记录
enable_episodic=true             # 是否启用情景记忆生成
enable_semantic=true             # 是否启用语义记忆生成
# ... 其他配置
```

## 记忆管理器

每种记忆类型都有对应的管理器类，负责对该类型记忆的增删改查操作。所有管理器都继承自 `MemoryManager<T>` 模板类，通过 `ORMWrapper` 与数据库进行交互，确保数据的一致性和完整性。

所有管理器由 `MemoryOrchestrator` 统一管理和协调，应用层（如 `ChatAgent`、`MemoryGenerationManager`）通过 `MemoryOrchestrator` 访问各类记忆，而不是直接操作数据库。

## 记忆检索

系统支持多种记忆检索方式：

1. **基于ID的检索**：直接通过记忆ID获取
2. **基于用户的检索**：查询特定用户的所有记忆
3. **基于会话的检索**：查询特定会话中的记忆
4. **基于条件的检索**：使用 `find()` 方法自定义查询条件
5. **溯源反查**：从长期记忆反查其生成来源

未来将支持：
- **基于向量的检索**：使用向量相似性搜索长期记忆
- **跨模态检索**：支持文本到图像、图像到文本的检索

## 记忆持久化

所有记忆数据都持久化存储在 SQLite 数据库中，确保系统重启后数据不会丢失。数据库文件统一存储在项目根目录下的 `database/` 目录中（默认为 `database/mirix.db`）。

## 历史设计说明

**已废弃的记忆类型**（v0.2 之前）：
- **资源记忆 (Resource Memory)**：已被 `MultimodalMemory` 替代
- **归档记忆 (Archival Memory)**：计划在未来版本中重新设计
- **工作记忆 (Working Memory)**：功能已整合到 `MessageMemory` 中
- **元记忆 (Meta-Memory)**：暂未实现

这些类型在当前版本中已经移除或重构，文档保留此说明以供参考历史设计思路。