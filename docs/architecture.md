# 系统架构文档

> 本文描述 **v2 端侧记忆系统（memolink）** 的分层架构。旧 MIRIX 桌面多模态系统已整体归入 `src/legacy/`（隔离不删），见文末「遗留代码」。权威设计方案与实测数据见 [memory_v2_design.md](memory_v2_design.md)。

## 概述

memolink 是跑在 **Jetson Orin** 上的端侧多类型 Agent 记忆系统：记忆的**提取 → 消解 → 存储 → 检索 → 更新 → 删除 → 版本回溯**全程本地完成，不依赖云端。

架构按**四层**组织，核心思想是：**应用层多样、可随时替换，记忆系统只对外暴露一个统一门面**。换应用（CLI → HTTP API → agent tool）只动 `app/`，核心与基础设施一行不改。

## 分层架构

```
┌───────────────────────────────────────────────────────────────┐
│  应用层  src/app/                              （多样、可替换） │
│    cli/memolink — 八命令 CLI；评测 harness 在 test/ + bench/    │
│    未来：HTTP API / agent tool / SDK 都只依赖下面的门面         │
└───────────────────────────┬───────────────────────────────────┘
                            │ 只依赖一个统一入口
┌───────────────────────────▼───────────────────────────────────┐
│  接口层  src/memory/api/                       （统一对外契约） │
│    MemorySystem — 门面：extract/ingest/recall/update/remove/   │
│                   list/history/stats，内部组装核心三件套        │
└───────────────────────────┬───────────────────────────────────┘
                            │ 组装
┌───────────────────────────▼───────────────────────────────────┐
│  核心实现层  src/memory/core/                  （记忆系统本体） │
│    MemoryStore     — memory 表 + 非破坏性版本化 CRUD + 类型特化检索 │
│    MemoryResolver  — 五操作三层消解（System-1 阈值 / System-2 本地 4B / 回退）│
│    MemoryExtractor — 受约束 JSON 提取（对话轮 → 候选事实）       │
└───────────────────────────┬───────────────────────────────────┘
                            │ 依赖
┌───────────────────────────▼───────────────────────────────────┐
│  基础设施层  src/infra/                        （可替换后端）   │
│    embedding/ — OnnxEmbedder · WordPieceTokenizer · TextEmbeddingGenerator │
│    llm/       — QwenAPI（本地 llama-server，OpenAI 兼容）       │
│    storage/   — StorageManager · TransactionManager · InitDB ·  │
│                 ConfigManager · VectorSerializer · TimeUtils    │
└───────────────────────────────────────────────────────────────┘

  遗留区  src/legacy/  —— 旧 MIRIX 桌面系统，隔离不删（见文末）
```

依赖方向严格自上而下：`app → api → core → infra`。`infra` 是最底层，被 `core` 与 `legacy` 共享；上层永不反向依赖下层的具体实现，应用层只认 `MemorySystem` 门面。

## 各层职责

### 应用层 `src/app/`
系统的各种「用法」，彼此独立、可增删替换。当前只有 `cli/memolink`（八命令：extract / ingest / recall / update / remove / list / history / stats）。评测 harness（`test/`、`bench/`）也属这一层——它们是记忆系统的消费方，不是记忆系统本身。未来加 HTTP API、agent tool、语言绑定，都只是在这一层新增一个依赖 `MemorySystem` 的薄壳。

### 接口层 `src/memory/api/`
只有一个类 `MemorySystem`，是记忆系统对外的**唯一契约**。它把核心三件套（Store / Resolver / Extractor）组装起来，对外暴露生命周期各阶段的方法。应用层拿到一个 `MemorySystem` 就能用，不必知道内部由几个类、按什么顺序构造。这是「应用可替换」得以成立的关键——没有这层，每个应用都得自己拼三个具体类。

### 核心实现层 `src/memory/core/`
记忆系统本体，三个部件各司一职：
- **MemoryStore**：独立的 `memory` 表（无 user/agent/session 外键），非破坏性版本化（`valid_until` / `superseded_by` / `supersedes`），显式 CRUD，以及类型特化检索（意图路由 + 同类型加权 + episodic 时间衰减）。
- **MemoryResolver**：五操作消解的三层判定——System-1 确定性阈值切 CREATE / 逐字 REINFORCE，模糊地带交 System-2 本地 4B 四选一，LLM 不可用则回退 CREATE（绝不覆盖或丢失）。
- **MemoryExtractor**：受约束 JSON 提取，把对话轮转成候选事实（每条带 type/owner/scope），规避开放式生成的编造。

### 基础设施层 `src/infra/`
可替换的后端能力，被核心层（和遗留代码）共享：
- **embedding/**：自写 C++ WordPiece 分词器 + ONNX Runtime INT8 推理（CPU-only，把 GPU 整块留给 LLM）。换语言只改 `config.ini` 的模型路径（中 bge-small-zh 512 维 ↔ 英 bge-small-en-v1.5 384 维，零代码改动）。
- **llm/**：`QwenAPI` 走本地 llama-server 的 OpenAI 兼容接口，供 System-2 消解与提取调用。
- **storage/**：SQLite 存取、事务、建表、配置、向量序列化。

## 数据流（记忆生命周期）

```
对话轮 ──①extract──▶ 候选事实 ──②ingest──▶ 五操作之一 ──③──▶ memory 表（版本化）
                     (MemoryExtractor)     (MemoryResolver)      (MemoryStore)
                                                                      │
query ──────────────────④recall（类型特化）◀────────────────────────┘
⑤update / remove / history（显式、可回溯）
```

1. **提取**：`MemoryExtractor` 把一轮对话转成受约束 JSON 候选事实；无可记事实则返回空（NOOP）。
2. **消解**：`MemoryResolver` 对每条候选做三层判定，产出 CREATE / UPDATE / REINFORCE / SUPERSEDE / NOOP 之一。
3. **存储**：`MemoryStore` 落库；SUPERSEDE 不删旧记忆，标记失效并建版本链。
4. **检索**：`MemoryStore.recallTyped` 按查询意图给同类型记忆加权、对 episodic 做时间衰减后重排。
5. **变更/回溯**：显式 `update` / `remove`，`history` 沿版本链回溯整条演化。

## 关键设计原则

1. **应用可替换**：应用层只依赖 `MemorySystem` 门面，不碰核心具体类。新增/替换应用是加一个薄壳，不动核心。
2. **非破坏性版本化**：矛盾更新走 SUPERSEDE（旧记忆失效但保留 + 双向链接），而非物理覆盖——记忆演化全程可回溯。
3. **端侧离线**：embedding 走 CPU、LLM 走本地 llama-server，全程不依赖云端；GPU 整块留给 LLM 是刻意的异构分配。
4. **基础设施可替换**：embedding 后端换语言只改配置（config-only swap，已实测中英互换 5/5）；存储/LLM 同理可换。
5. **遗留隔离不删**：旧 MIRIX 桌面系统整体移入 `src/legacy/`，与活跃四层物理隔开，既保留代码又不污染主线。

## 遗留代码 `src/legacy/`

旧 MIRIX 桌面多模态系统（约占仓库 2/3 行数），整体隔离、默认不参与端侧构建（桌面部分由 `MEMOLINK_ENABLE_CAPTURE/GUI/API` 开关控制，默认 OFF）：

- **core-v1/**：v1 记忆路径——12 个分类型 manager + `MemoryOrchestrator` + `RecallMemoryManager`（用旧 `recall_memory` 表）。已被 v2 的 `MemoryStore` 取代；其中 12 个 manager 为零调用死代码。`test_memory_orchestrator` / `test_retrieval_eval`（早期 P3/P4 三域检索评测）仍依赖它。
- **infra-alt/**：旧基础设施备选——`VectorSearchManager` / `EmbeddingIndexManager` / `CLIPProcessor` / `ModelManager`。可视为可替换后端的另一种实现（如 CLIP 多模态、向量索引）。
- **agents/**：9 个 MIRIX 上游 agent 封装。**未来应用候选**——重写成调 `MemorySystem` 即可成为 agent 应用。
- **desktop/**：桌面前端——`capture`（相机/音频/截屏）/ `gui`（Qt）/ `api`（HTTP server）/ `cli`（旧交互式 CLI）。**未来应用候选**——它们本就是记忆系统的不同前端，重写成依赖 `MemorySystem` 的薄壳即可迁入 `app/`。
- **storage-stub/**：一个已废弃的 `StorageManager` 桩（真现在 `infra/storage/`），仅保留占位。

> 说明：legacy 内的旧文档（`database.md` / `memory.md` / `multimodal.md` / `improvements.md`）与装饰性 "MIRIX" 字样按「隔离即可、不强制清理」保留原状。
