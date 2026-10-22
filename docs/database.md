# 数据库设计（v2 端侧记忆系统）

> 本文描述 **memolink v2 记忆系统**的数据模型。v2 只用一张 `memory` 表；旧 MIRIX v1 的 14 张表（user/agent/session/recall_memory/…）已随 legacy 代码移出构建，其旧文档归档于 `src/legacy/docs/`。权威设计与依据见 [memory_v2_design.md](memory_v2_design.md) §10。

## 概述

v2 用 SQLite 存储记忆，核心是单张 `memory` 表。存储层由 `infra/storage/` 提供：

- **StorageManager**：数据库连接与查询执行（`initialize()` 会开 `PRAGMA foreign_keys=ON`）。
- **InitDB**：`initializeDatabase(dbPath)` 建 `memory` 表 + 索引（v2 只建这一张表）。
- **VectorSerializer**：embedding 向量与 BLOB 的序列化 / 反序列化。
- **ConfigManager**：读 `config.ini`（含 `[edge]` 端侧模型路径）。

数据库文件路径由 `config.ini` 的 `[database] db_path` 指定（默认 `memolink.db`）。

## `memory` 表

```sql
CREATE TABLE IF NOT EXISTS memory (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    type            TEXT NOT NULL,        -- semantic | episodic | procedural
    content         TEXT NOT NULL,
    embedding       BLOB,                 -- VectorSerializer 序列化的向量
    embed_model     TEXT,                 -- 模型版本 tag，换模型时检出失配
    strength        REAL DEFAULT 1.0,     -- REINFORCE 累加，强度衰减淘汰用
    access_count    INTEGER DEFAULT 0,
    -- 治理三元标签（Q7）
    owner           TEXT DEFAULT 'user',      -- user | agent | system
    scope           TEXT DEFAULT 'private',   -- private | shared
    authority       TEXT DEFAULT 'asserted',  -- asserted | observed | inferred
    -- 非破坏性版本化（Q1）
    valid_from      TEXT,
    valid_until     TEXT,                 -- NULL = 当前有效
    superseded_by   INTEGER,              -- 被哪条取代（双向链）
    supersedes      INTEGER,              -- 取代了哪条
    -- 溯源
    source_session  TEXT,
    source_turn     INTEGER DEFAULT 0,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_memory_type_valid     ON memory(type, valid_until);
CREATE INDEX IF NOT EXISTS idx_memory_superseded_by  ON memory(superseded_by);
CREATE INDEX IF NOT EXISTS idx_memory_scope          ON memory(scope);
```

### 字段分组

| 组 | 字段 | 作用 |
|---|---|---|
| 基本 | `id` / `type` / `content` | 记忆主键、类型（3 类）、正文 |
| 向量 | `embedding` / `embed_model` | 检索用向量（BLOB）+ 生成它的模型 tag |
| 强度 | `strength` / `access_count` | REINFORCE 提升 strength；强度衰减淘汰按 `strength × exp(-age/τ)` |
| 治理 | `owner` / `scope` / `authority` | 三元标签过滤（Q7）；`scope=private` 命中强制端侧 |
| 版本化 | `valid_from` / `valid_until` / `superseded_by` / `supersedes` | 非破坏性版本化：SUPERSEDE 标旧记忆失效并建双向链，`history` 可回溯 |
| 溯源 | `source_session` / `source_turn` / `created_at` / `updated_at` | 来源与时间戳；`updated_at` 是强度衰减的 age 基准 |

## 关键设计：无 user/agent/session 外键

`memory` 表**刻意不带** user/agent/session 外键。原因：v1 的 `recall_memory` 对这三张表都有外键，配合 `StorageManager::initialize()` 里的 `PRAGMA foreign_keys=ON`，曾让 P3 首跑 40/40 条写入全部违约失败（修法见 legacy 的 `MemoryOrchestrator::ensureDefaultEntities`）。v2 表自成一体、不依赖那三张表，从 schema 层根除这类失败。

## 类型与检索

`type` 取 3 类（认知科学标准分法），并**真正驱动行为**（不是标签摆设）：

- **semantic**：事实与偏好，可被矛盾 → SUPERSEDE。
- **episodic**：带时间戳的事件，不可变；检索时做时间衰减、淘汰时优先。
- **procedural**：技能/规则，可被精化 → UPDATE；淘汰时最后。

类型特化检索（意图路由 + 同类型加权 + episodic 时间衰减）见 [memory_v2_design.md](memory_v2_design.md) §7，实现在 `MemoryStore::recallTyped`。

## 向量检索

检索是对 `memory` 表中存储的 embedding 做**暴力余弦相似度**（`MemoryStore::searchByEmbedding`），未链接 FAISS。实测在 agent 记忆量级（1e3~1e4）下精确检索足够（0.79ms@1万条、recall 1.0），而 HNSW 建索引要 36s@10万条，故刻意不接 FAISS（连带不需要 Eigen3）。
