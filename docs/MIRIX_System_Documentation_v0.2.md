# MIRIX系统文档 v0.2

## 1. 系统概述

MIRIX是一个先进的多模态集成推理系统，结合了大型语言模型、视觉处理和记忆管理功能，旨在实现智能代理的高级认知能力。

### 1.1 核心功能

- 多模态数据捕获与分析（屏幕、摄像头、麦克风）
- 基于SQLite的本地数据库存储与ORM操作
- 记忆生成、分类与调度
- 向量嵌入与相似性搜索
- API服务接口与命令行交互
- 智能代理对话与上下文构建

### 1.2 目标用户

- AI研究人员
- 开发者
- 需要多模态推理能力的应用系统

## 2. 系统架构

### 2.1 整体架构

MIRIX采用分层架构模式，包含以下层级：
1. 用户接口层（CLI/API）
2. 模型与AI管理层
3. 记忆处理层
4. 存储管理层
5. 数据库层

### 2.2 关键技术决策

- 使用C++17标准开发，保证性能与现代特性支持
- 采用sqlite_orm作为ORM框架，提升数据库访问安全性与可维护性
- 所有第三方依赖本地化管理，按平台分离（macOS/Linux）
- 核心模块解耦，通过清晰的目录结构组织代码

### 2.3 架构模式

- 分层架构：明确划分各功能层级
- 依赖注入：通过管理器模式组织数据库访问
- 观察者模式：事件系统用于模块间通信（EventSystem）

### 2.4 设计模式

- 工厂模式：用于内存生成器（MemoryGenerationManager）
- 策略模式：不同类型的内存生成策略（如Episodic、Semantic等）
- 单例模式：配置管理、存储管理等全局访问点
- 组合模式：多模态数据管理与记忆编排

## 3. 主要组件交互

1. 用户接口（CLI/API）触发Agent或直接调用模型
2. Agent调用MemoryOrchestrator进行记忆读写
3. MemoryOrchestrator通过各MemoryManager操作数据库
4. 数据库操作由ORMWrapper封装，基于sqlite_orm执行SQL
5. 模型模块（CLIP、Embedding）生成向量并存入数据库
6. 定时任务通过MemoryGenerationScheduler触发记忆生成

## 4. 技术栈

### 4.1 核心技术

| 类别 | 技术 |
|------|------|
| 前端 | 无独立前端，提供CLI和API接口 |
| 后端 | C++17, CMake构建系统 |
| 数据库 | SQLite3 + sqlite_orm |
| AI模型 | CLIP, Qwen API, 文本嵌入模型 |
| 计算机视觉 | OpenCV |
| 音频处理 | PortAudio |
| 网络请求 | cURL |
| JSON处理 | JSONCPP |
| 向量搜索 | Faiss |
| GUI（可选） | Qt5 |
| 加密通信 | OpenSSL |

### 4.2 版本和兼容性要求

- CMake ≥ 3.10
- C++17 支持
- OpenCV（版本未指定）
- SQLite3（版本未指定）

## 5. 系统模块详解

### 5.1 智能代理模块 (`src/agents/`)

包含各种智能代理实现，如ChatAgent、MemoryAgent等，负责处理用户请求和协调其他模块。

### 5.2 API模块 (`src/api/`)

提供RESTful API服务，支持外部系统调用MIRIX的功能。

### 5.3 多模态数据捕获模块 (`src/capture/`)

负责捕获多种类型的数据：
- 音频数据捕获
- 视频数据捕获
- 屏幕活动跟踪

### 5.4 命令行接口模块 (`src/cli/`)

实现命令行交互界面，方便用户直接操作系统。

### 5.5 核心系统组件 (`src/core/`)

包含系统的核心功能，如事件系统等。

### 5.6 数据库模块 (`src/database/`)

#### 5.6.1 核心组件 (`src/database/core/`)

- ORMWrapper：sqlite_orm的封装层，提供类型安全的操作接口
- StorageManager：存储管理器，管理数据库连接和ORM实例

#### 5.6.2 初始化 (`src/database/initialization/`)

数据库初始化逻辑和配置管理。

#### 5.6.3 管理器 (`src/database/managers/`)

各类记忆管理器：
- EpisodicMemoryManager：情景记忆管理器
- KnowledgeBaseManager：私密信息库管理器
- SemanticMemoryManager：语义记忆管理器
- SessionManager：会话管理器
- SkillManager：技能管理器
- UserModelManager：用户模型管理器
- UserProfileManager：用户档案管理器

### 5.7 记忆模块 (`src/memory/`)

#### 5.7.1 上下文构建 (`src/memory/context/`)

上下文构建逻辑，用于维护对话和任务上下文。

#### 5.7.2 记忆生成 (`src/memory/generation/`)

记忆生成逻辑与分类，包括不同类型记忆的生成策略。

#### 5.7.3 调度 (`src/memory/scheduling/`)

记忆生成调度器，控制何时以及如何生成不同类型的记忆。

### 5.8 模型模块 (`src/models/`)

模型接口封装，包括：
- CLIP模型接口
- Qwen大语言模型接口
- 文本嵌入模型接口
- 向量搜索接口

### 5.9 工具模块 (`src/utils/`)

各种实用工具类，如：
- 时间处理工具
- 向量序列化工具

## 6. 数据模型

### 6.1 统一记忆模型架构

MIRIX采用统一的记忆模型架构，所有记忆类型都遵循相同的基础结构，但在具体实现上有所区别。

### 6.2 主要记忆类型

1. **情景记忆(Episodic Memory)**：存储用户的个人经历和体验
2. **语义记忆(Semantic Memory)**：存储事实和概念知识
3. **过程记忆(Procedural Memory)**：存储技能和习惯性行为
4. **工作记忆(Working Memory)**：临时存储和处理信息
5. **元记忆(Meta-Memory)**：关于记忆的知识和控制

## 7. 开发环境与部署

### 7.1 必需工具

- CMake ≥ 3.10
- 支持C++17的编译器（如g++、clang++）
- make 构建工具
- Git（用于获取依赖）

### 7.2 可选工具

- Qt5（用于GUI功能）
- Python（脚本使用，如模型下载）

### 7.3 搭建开发环境

```bash
# 1. 克隆项目
git clone https://github.com/xxx/mirix-cpp.git
cd mirix-cpp

# 2. 准备依赖（Linux示例）
./scripts/prepare_linux_libs.sh

# 3. 构建项目
mkdir build && cd build
cmake ..
make
```

### 7.4 构建、部署和运维

- 构建命令: `cmake .. && make`
- 本地运行测试: `./mirix_test_sqlite_orm`, `./mirix_test_updated_orm_managers`
- 部署: 编译后的二进制文件可直接运行，依赖库需随包分发
- 数据库存放路径: `database/mirix.db`
- 日志与捕获数据: `captures/` 目录下

## 8. v0.2版本重要更新

### 8.1 ORM架构全面升级

在v0.2版本中，我们完成了从原始SQL操作到ORM模式的全面迁移：

1. **完全移除原始SQL操作**
   - 删除了所有直接使用`sqlite3_prepare_v2`和`sqlite3_exec`的代码
   - 移除了基于原始SQL的SystemEventManager实现
   - 重构了SessionManager以使用ORMWrapper接口

2. **统一ORM接口**
   - 所有数据库操作现在都通过ORMWrapper进行
   - ORMWrapper封装了sqlite_orm库，提供了类型安全的操作接口
   - 实现了统一的CRUD操作接口

3. **改进的错误处理**
   - 使用std::optional和std::variant改进了返回值处理
   - 增强了异常处理机制

### 8.2 架构优化

1. **更好的模块解耦**
   - 通过ORMWrapper实现了数据访问层与业务逻辑层的分离
   - 各MemoryManager通过统一接口访问数据库

2. **增强的类型安全**
   - 利用C++17的特性提高代码安全性
   - 通过编译时检查减少运行时错误

### 8.3 代码质量提升

1. **移除弃用代码**
   - 删除了不再使用的原始SQL实现
   - 清理了相关的历史遗留代码

2. **标准化编码**
   - 统一了数据库操作接口
   - 规范了错误处理方式

## 9. 已知问题与限制

### 9.1 技术限制

- sqlite_orm库中使用了一些C++17中已弃用的特性，产生编译警告
- Windows支持仍在开发中

### 9.2 性能要求

- 实时音视频捕获与处理，需低延迟响应

### 9.3 安全要求

- 使用OpenSSL进行安全通信
- 本地数据库建议加密（未明确实现）

## 10. 未来规划

### 10.1 短期目标

1. 解决第三方库的弃用警告问题
2. 增强Windows平台支持
3. 完善测试覆盖范围

### 10.2 长期目标

1. 实现更复杂的记忆关联和检索机制
2. 增强多模态数据处理能力
3. 提升系统智能化水平和自适应能力

## 11. 项目目录结构

```
mirix-cpp/
├── src/
│   ├── agents/           # 智能代理模块
│   ├── api/              # API服务接口
│   ├── capture/          # 多模态数据捕获
│   ├── cli/              # 命令行接口
│   ├── core/             # 核心系统组件
│   ├── database/         # 数据库相关
│   │   ├── core/         # ORM封装、存储管理
│   │   ├── initialization/ # 数据库初始化
│   │   └── managers/     # 各类记忆管理器
│   ├── memory/           # 记忆处理模块
│   │   ├── context/      # 上下文构建
│   │   ├── generation/   # 记忆生成逻辑
│   │   └── scheduling/   # 记忆生成调度器
│   ├── models/           # 模型接口封装
│   └── utils/            # 工具类
├── test/                 # 单元测试与集成测试
├── libs/                 # 平台相关依赖库
├── docs/                 # 文档
├── scripts/              # 辅助脚本
├── database/             # 数据库文件存放目录
└── models/               # 模型配置文件
```

---
版本: v0.2
更新日期: 2025-12-03

## 12. v0.3 版本规划与演进方向

### 12.1 v0.2 → v0.3 已完成项

以下功能已在当前版本中实现：

#### 12.1.1 记忆系统重构
- ✅ **多模态记忆表统一**：将旧的 `resource_memory` 重构为 `multimodal_memories`，统一存储图像、音频、截图等多模态数据
- ✅ **消息处理状态跟踪**：为 `MessageMemory` 增加 `processed` 字段，标记消息是否已参与长期记忆生成
- ✅ **记忆溯源机制**：实现 `MemoryProvenance` 表，记录长期记忆与消息/多模态来源的多对多关系
- ✅ **MemorySource 抽象**：统一抽象消息和多模态数据为记忆来源，支持灵活的记忆生成流程
- ✅ **全类型记忆溯源**：将溯源机制从 episodic/semantic 扩展到 core/procedural/secret_vault 等所有长期记忆类型

#### 12.1.2 多模态处理优化
- ✅ **多模态大模型优先**：在图像、截图处理中优先使用多模态大模型（qwen2.5-omni）生成自然语言场景描述，写入 `description` 字段
- ✅ **降级策略完善**：多模态模型 → CLIP模型 → 默认模板的三级降级策略
- ✅ **语义驱动记忆生成**：`multimodal_memories.description` 作为文本来源参与长期记忆生成

#### 12.1.3 配置驱动与灵活性提升
- ✅ **记忆生成配置化**：通过 `config.ini` 的 `[memory_generation]` 节配置记忆生成行为
- ✅ **可配置生成策略**：支持配置消息阈值、最大来源数、各类记忆开关等参数
- ✅ **ConfigManager 扩展**：增加读取记忆生成配置的方法

#### 12.1.4 架构优化与文档更新
- ✅ **ORM封装完善**：ORMWrapper 统一管理数据库操作，提供类型安全的接口
- ✅ **文档同步更新**：更新 memory.md、multimodal.md、architecture.md，反映当前架构状态

### 12.2 v0.3 待完成改进项

以下是后续版本的重点改进方向：

#### 12.2.1 记忆生成策略优化（高优先级）

**问题**：当前 `collectSources()` 可能为每条长期记忆挂载过多来源，缺乏细粒度控制。

**改进方向**：
- 在 `collectSources()` 中加入 session 过滤，仅收集当前会话或最近会话的来源
- 设置类型配额：例如最近5条消息 + 3条截图 + 2条音频，避免来源爆炸
- 提供配置项控制来源选择策略（按时间窗口、按数量、按相关性等）

**对应任务**：i1Refine_ProvenanceScope

#### 12.2.2 Demo配置界面增强（中优先级）

**问题**：当前 demo 菜单未暴露记忆生成配置项，调整配置需手动修改 `config.ini`。

**改进方向**：
- 在 demo 配置菜单中增加记忆生成配置子菜单
- 支持在运行时调整 `message_threshold`、`max_sources_per_generation`、`enable_*` 开关
- 提供配置预览和保存功能

**对应任务**：i2Demo_ConfigUI

#### 12.2.3 记忆生成入口统一（中优先级）

**问题**：当前存在多个记忆生成入口（`generateMemories(vector<string>)` 等），可能导致调用混乱。

**改进方向**：
- 标记 `generateMemories(vector<string>)` 为 legacy API
- 在 MemoryOrchestrator 中新增统一高层 API：`triggerMemoryGeneration()`
- 该 API 自动收集未处理消息、多模态数据，调用 MemoryGenerationManager

**对应任务**：i3Unify_GenEntry

#### 12.2.4 事务支持完善（高优先级）

**问题**：记忆生成 + 溯源记录 + 消息标记等批量操作缺乏事务包裹，可能导致数据不一致。

**改进方向**：
- 在 ORMWrapper 中补完事务封装，提供 `beginTransaction()`、`commit()`、`rollback()` 接口
- 对记忆生成流程使用事务包裹：
  ```cpp
  transaction.begin();
  try {
      // 生成长期记忆
      // 写入溯源记录
      // 标记消息为已处理
      transaction.commit();
  } catch (...) {
      transaction.rollback();
  }
  ```

**对应任务**：i4Transaction_Support

#### 12.2.5 API健壮性提升（高优先级）

**问题**：QwenAPI 调用缺乏完善的错误处理和响应校验。

**改进方向**：
- 为 API 请求引入 `validate()` 方法，在发送前校验参数
- 响应解析增加字段校验（检查必需字段是否存在、类型是否正确）
- 错误恢复策略：重试机制、降级策略、异常捕获与日志记录

**对应任务**：i5API_Robustness

#### 12.2.6 长期记忆向量化与检索（中优先级）

**问题**：当前长期记忆（Episodic、Semantic、Core等）缺乏向量索引，无法进行语义检索。

**改进方向**：
- 为 Episodic、Semantic、Core 等长期记忆建立向量索引（使用 TextEmbeddingGenerator）
- 在 ContextBuilder 中引入长期记忆检索：根据查询向量召回相关长期记忆
- 提供混合检索策略：短期记忆（消息） + 长期记忆（情景、语义）

**对应任务**：i6LongTerm_VectorIndex

### 12.3 长期演进方向

#### 12.3.1 记忆演化与遗忘机制
- 实现记忆重要性评分，自动淡化不重要的记忆
- 记忆合并：将相似的情景记忆合并为更抽象的语义记忆
- 记忆冲突检测与解决

#### 12.3.2 跨会话记忆关联
- 建立会话间的记忆关联图谱
- 支持跨会话的主题追踪和知识积累

#### 12.3.3 多模态理解增强
- 引入更先进的多模态模型（如 GPT-4V、Gemini）
- 支持视频流处理和实时场景理解
- 音频转文本（语音识别）与情感分析

#### 12.3.4 隐私与安全
- 本地数据库加密（SQLite加密扩展）
- 敏感信息脱敏与访问控制
- 记忆导出与备份机制

---
v0.3 规划更新日期: 2025-12-23