# 端侧 Agent 记忆系统 v2 — 设计方案

- **状态**：待开工（P0–P5 已完成并合并进 main，见 [`orin_edge_plan.md`](orin_edge_plan.md)）
- **定案**：2026-10-21
- **引用策略**：核心机制**锚定已落地系统**（以 GitHub star 与持续提交为证）；2026 预印本**仅作旁证**，逐条标注「预印本 / 有无代码」

---

## 0. 为什么需要 v2

P0–P5 交付的是「能在 Orin 上离线跑通的记忆检索管线」。2026-10-21 三轮代码审计暴露出它**还不是一个记忆系统**：

| 缺口 | 实测证据 |
|---|---|
| 无更新 | 全仓 13 个 manager 中只有 4 个有 update 方法，且都没被编排器调用；`RecallMemoryManager` 里唯一含 "update" 的是 `updated_at` **字段**，不是方法 |
| 无定向删除 | 唯一一条 `DELETE` 在 `evictType` 里，是容量触发的 FIFO，不能按 id 删 |
| 无提取 | 记忆是现成文本喂进来的，没有「从对话抽取候选记忆」这一环 |
| 类型不起作用 | 4 个类型只影响淘汰容量与展示；检索全类型一起搜，类型不参与任何路由 |
| 复用被夸大 | 编排器**只调用 `RecallMemoryManager`（464 行）**；其余 12 个 manager（2,146 行）编进库但从未调用 |
| 测试覆盖不足 | ~13 个公开方法只测过 6 个 |
| 评测是自证 | 语料与标注均本人手写，GT 自定义 |

v2 的目标是把这些补齐，并且**用公开基准替代自证评测**。

---

## 1. 引用依据分级

### 1.1 第一档：已落地系统（核心机制的依据）

| 系统 | ⭐ star | 最近提交 | 本项目借鉴的机制 |
|---|---|---|---|
| [`mem0ai/mem0`](https://github.com/mem0ai/mem0) | **67,552** | 2026-10-21 | 两阶段管线（提取 → 更新）；top-s 相似检索后决策；ADD/UPDATE/DELETE/NOOP |
| [`topoteretes/cognee`](https://github.com/topoteretes/cognee) | 32,605 | 2026-10-21 | 记忆平台化形态（参照，不直接借鉴机制） |
| [`getzep/graphiti`](https://github.com/getzep/graphiti) | **32,090** | 2026-10-21 | 时序知识图谱、**边失效而非删除** |
| [`letta-ai/letta`](https://github.com/letta-ai/letta)（前 MemGPT） | **25,211** | 2026-10-18 | core / archival / recall 三层记忆分层 |
| [`Mirix-AI/MIRIX`](https://github.com/Mirix-AI/MIRIX) | **3,457** | 2026-10-08 | 多类型记忆分类（**本仓库上游**） |

### 1.2 第二档：2026 预印本（仅作旁证，**均无同行评审**）

> ⚠️ 核实结论：抽查 8 篇，`gh search repos <arxiv-id>` **全部 0 命中**；只有 FEB-30（`amazon-science/feb-30-benchmark`）与 Bemula（`Nboxff/bemula-benchmark`）有 benchmark 仓。**因此这些只用于「开放问题证据」与「近期细化方向」，不作为设计依据。**

| 论文 | arXiv | 日期 | 代码 | 用作 |
|---|---|---|---|---|
| FEB-30 | `2610.22209` | 10-15 | ✅ Amazon | create/update/reinforce/supersede 四分类；现有系统仅 20% 正确 |
| TriMEM | `2610.11573` | 10-08 | ❌ | 类型特化检索 +16.18% Recall@1；证明单一策略有精度上界 |
| Mnemon | `2609.36059` | 09-28 | ❌ | System-1 小模型快判断 / System-2 大模型慢思考 |
| MemFit | `2610.00872` | 10-01 | ❌ | 刻意 LLM-free 的写入与检索 |
| Mem++ | `2610.02002` | 10-01 | ❌ | 被取代版本保留、读取时选择 |
| TEM | `2610.21900` | 10-15 | ❌ | 版本化 + 显式标注历史记录 |
| PACMI | `2610.05732` | 10-05 | ❌ | 四态有效性格 + 级联失效 |
| RSPM | `2610.25561` | 10-18 | ❌ | 睡眠期蒸馏 + 衰减未强化规则 |
| Bemula | `2610.18052` | 10-13 | ✅ | **无系统遗忘请求端到端通过率 >1.5%** |
| MemCut | `2610.21405` | 10-15 | ❌ | LLM 引导删除后仍 **39–76% 可恢复** |
| SAFE-Mem | `2610.22959` | 10-20 | ❌ | kind / authority / scope 治理标签 |
| Owner attribution | `2610.25707` | 10-18 | ❌ | 写入时绑 owner，跨用户误用 92→1 / 305 |

### 1.3 评测基准（第一档）

| 基准 | ⭐ star | 许可 | 关键价值 |
|---|---|---|---|
| [`xiaowu0162/LongMemEval`](https://github.com/xiaowu0162/LongMemEval) | 1,163 | **MIT** | 500 题、5 类能力；**自带 turn 级 `has_answer` 与 session 级 `answer_session_ids` 召回金标** |
| [`snap-research/locomo`](https://github.com/snap-research/locomo) | 1,235 | 见仓内 | 超长对话记忆 |

---

## 2. 架构总览

```
                        ┌──────────── Jetson Orin（全程离线）────────────┐
 对话轮 (m_{t-1}, m_t) ──┤                                              │
                        │  ① 提取 EXTRACT                               │
                        │     本地 4B，受约束结构化输出（JSON）           │
                        │     → 候选事实集 Ω                             │
                        │                                              │
                        │  ② 消解 RESOLVE（对每个 ω ∈ Ω）                │
                        │     embed(ω) → top-s 相似检索                  │
                        │     ├ System-1 确定性快路径（不调模型）          │
                        │     │   score < θ_new        → CREATE          │
                        │     │   score ≥ θ_dup 且等价  → REINFORCE       │
                        │     └ System-2 模糊地带（本地 4B，四选一）        │
                        │         {UPDATE, SUPERSEDE, REINFORCE, NOOP}    │
                        │         + target_id；超时/不可用 → 保守回退      │
                        │                                              │
                        │  ③ 存储 STORE                                 │
                        │     SQLite `memory` 表 + embedding BLOB         │
                        │     非破坏性版本化（valid_until/superseded_by）  │
                        │                                              │
 查询 q ────────────────┤  ④ 检索 RECALL                                │
                        │     类型意图判定 → 类型特化加权 → 暴力余弦       │
                        │     → 只返回当前有效（可选 --include-history）   │
                        │                                              │
                        │  ⑤ 治理 GOVERN                                │
                        │     owner/scope 标签；隐私命中 → 强制端侧        │
                        │                                              │
                        │  ⑥ 遗忘 FORGET                                │
                        │     分类型容量 + 强度衰减 + 显式 remove(id)      │
                        └──────────────────────────────────────────────┘
```

---

## 3. 记忆生命周期：五操作 + 显式删除

| 操作 | 语义 | 触发 | 依据 |
|---|---|---|---|
| **CREATE** | 新信息，生成新 id | 无相似候选 | Mem0 ADD |
| **UPDATE** | 用**补充**信息增强已有记忆，**保留原 id** | 相似且不矛盾、信息量更大 | Mem0 UPDATE（Algorithm 1：仅 `InformationContent(f) > InformationContent(m_i)` 时触发） |
| **REINFORCE** | 同一事实再次出现 → 提升 `strength`，**不新增记录** | 近重复 | FEB-30（预印本） |
| **SUPERSEDE** | 新事实**矛盾**于旧 → 旧的标失效、新的写入并互链 | 相似且矛盾 | Graphiti 边失效；Mem0ᵍ 软失效 |
| **NOOP** | 无需改动 | 已存在或无关 | Mem0 NOOP |
| **REMOVE** | 显式按 id 删除（**记录级，非 unlearning**） | 用户/上层显式请求 | 见 §15 诚实边界 |

**为什么把 SUPERSEDE 从 DELETE 里分出来**：Mem0 论文 Algorithm 1 的 DELETE 分支只写 `M ← M ∖ {m_i}`，**没有插入新的矛盾事实**，论文自身未闭合。把「矛盾」独立成 SUPERSEDE（旧失效 + 新写入 + 双向链接）绕开这个缺口，且与 Graphiti 的边失效一致。

---

## 4. 非破坏性版本化

依据：Graphiti（32,090⭐，时序图 + 边失效）；旁证 Mem++ / TEM / PACMI（预印本，四篇独立收敛到「保留被取代记录」）。

- `valid_from` / `valid_until`：双时间轴，`valid_until IS NULL` = 当前有效
- `superseded_by` / `supersedes`：双向链接，可回溯演化链
- 检索**默认只返回当前有效**；`--include-history` 可查「偏好如何变化」
- **物理删除只在显式 REMOVE 时发生**

---

## 5. 三层消解（端侧核心差异化）

Mem0 每轮 ingest 要**两次 GPT-4o-mini 调用**（提取 + 消解）。端侧照搬不可行：本地 4B 单次生成实测 ~920ms，且 **P2 已实测它在开放式综合问题上编造**（答出记忆里不存在的「早高峰+1分钟」）。

**设计**：

| 层 | 判定 | 成本 | 依据 |
|---|---|---|---|
| **System-1** | 相似度阈值：`< θ_new`(≈0.50) → CREATE；`≥ θ_dup`(≈0.999，近逐字) → REINFORCE | ~4ms，**不调模型** | MemFit（LLM-free 写入，预印本） |
| **System-2** | 模糊地带 `θ_new ≤ score < θ_dup` 调本地 4B，**输出受约束为四选一 + target_id**（结构化，非自由生成） | ~900ms | Mnemon System-1/2 分工（预印本）；Mem0 tool-call 决策（已落地） |
| **回退** | 模型不可用/超时/解析失败 → 保守取 **CREATE**（绝不覆盖或丢失旧记忆，冗余可事后去重） | ~0 | 端侧可用性要求 |

**实测修正（2026-10-21，dog，bge-small INT8，30 标注场景，`bench/calibrate_thresholds.py`）**：原设计假设「System-1 覆盖率 70–80%」**被标定数据推翻**，阈值改为从实测分布推导：

- **θ_new ≈ 0.50 能干净切出 CREATE**（CREATE 最高 0.442 < SUPERSEDE 最低 0.556）——这是 System-1 唯一可靠的确定性判定。
- **UPDATE/SUPERSEDE/REINFORCE 全挤在 0.77–1.0**，且**新颖度信号（候选新 token 占比）也救不了**：UPDATE 中位 0.448 vs SUPERSEDE 中位 0.477 几乎完全重叠；REINFORCE 虽中位低（0.236）但最高 0.538 又撞进 UPDATE 区间。矛盾事实因共享主题而向量相近、改写又天然带新词——**纯确定性信号无法消解矛盾/改写/补充**。Mem0 的 `InformationContent(f) > InformationContent(m_i)` 是语义量，token 重叠代理实现不了。
- **System-1 实测覆盖率 ≈ 32%**（7 CREATE + 1 逐字 REINFORCE / 25 可记），其余 68% 必须走 System-2。端侧写入均成本因此**不是 ~5ms 而是 ~0.32×4ms + 0.68×900ms ≈ 613ms**——仍优于 Mem0 的两次云端 GPT-4o-mini 调用，但优势来自「本地 4B 不出端 + 隐私」，**不是「快两个数量级」**。注：32% 是在**均衡**场景集上测的；真实 ingest 流若 CREATE 占多数，覆盖率会更高，但真实分布尚未测量，故撤回 70–80% 的猜测。

**Q2 消解器实测（2026-10-21，dog，qwen3-4b 本地，30 场景，`test_memory_resolver`）**——「约束成四选一就能规避编造」的假设得到**部分验证**：

- **五分类准确率 70%（21/30）**；剔除 NOOP 后**可记四类 84%（21/25）**。**目标定位 14/14**（动作判对时 target_id 全对）。
- **逐类**：CREATE 7/7、UPDATE 6/6、SUPERSEDE 5/6、**REINFORCE 3/6（最弱）**、NOOP 0/5。
- **System-2 输出始终是合法 JSON、target_id 始终落在喂进去的候选集内**，未出现 P2 那种开放式编造——约束结构化输出确实规避了自由生成的编造。
- **REINFORCE 是 4B 模型的系统性弱点**：等价重述/信息更少的子集被持续误判成 UPDATE（模型把任何措辞变化当成「新增细节」）。三版 prompt 实测：平衡定义 70%、加 Mem0 判定顺序反而 60%（顺序把模型带偏成全判 UPDATE）、平衡+具体细节判据 70%——**非 prompt 能稳定修复，是端侧小模型的固有上限**。
- **NOOP 0/5 是架构边界不是 bug**：寒暄类候选相似度低（0.23–0.39），和 CREATE 一样落进 System-1 快路径被 CREATE。NOOP 过滤是提取阶段（Q4）的职责——消解器拿到的本应是已过滤的可记候选；要消解器自己抓低相似度 NOOP，就得把 CREATE 快路径也送 LLM，43% 覆盖率随即归零。

---

## 6. 提取阶段（v1 完全缺失的一环）

依据：Mem0 提取阶段（已落地，67,552⭐）——触发单位是**消息对** `(m_{t-1}, m_t)`，上下文 = 异步刷新的会话摘要 `S` + 最近 `m=10` 条消息，经 LLM 函数 `φ(P)` 输出候选事实集 `Ω`。

**端侧适配**：
- 输出**受约束为 JSON 数组**（每条含 `text` / `type` / `owner` / `scope`），不是自由文本
- 会话摘要 `S` 异步刷新、不在关键路径（照 Mem0）
- **提取准确率必须实测**：用 LongMemEval 的 `has_answer` turn 标注做金标，测「该抽的抽到了吗、抽出来的有没有编造」

**Q4 实测（2026-10-21，dog，qwen3-4b，LongMemEval oracle 分层子集 30 题/5 题型，`bench/extract_eval.py`）**：

| 指标 | 结果 |
|---|---|
| JSON 解析成功率 | **123/125 = 98.4%**（结构化输出约束可靠） |
| 编造率（落地率<0.7） | **27/730 = 3.7%（上界）** |
| 答案查全（覆盖≥0.6） | **17/30 = 56.7%** |
| 单题均耗时 | 37.3s（分块后） |

分题型查全：multi-session 4/5、single-session-assistant 4/5、single-session-user 3/5、temporal-reasoning 3/5、knowledge-update 2/5、single-session-preference 1/5。

**关键结论**：
- **编造率 3.7% 是上界**：低落地率样本人工核验几乎全是 assistant 建议的**改写**或温和推断（如「User values perfection」），**不是凭空捏造**——受约束 JSON 提取确实规避了 P2 实测的开放式编造。
- **分块是必需的**：llama-server `-c 4096`，单段长对话（preference 题 ~6.4k tokens）一次性喂会 HTTP 400；按 8000 字符分块后解析成功率 73%→98.4%。
- **查全短板有明确归因**：knowledge-update 漏「最新值」（关联 Q2 SUPERSEDE，提取抓到中间值）；single-session-preference 金标是**隐式元偏好**（需推断），提取器按 no-infer 规则正确拒绝——是任务边界不是缺陷。
- **user-focus 提示词是净负面**（实测对照）：加「优先抽用户事实、跳过 assistant 讲解」后 assistant 题型 4/5→2/5、总查全 56.7%→43.3%，已回退——别凭直觉给提取加偏向，用数据说话。
- **度量是下界**：token 重叠有单复数/同义盲区，人工核验发现个别 cov<0.6 题其实抽到了（如「45-minute commute each way」）。

---

## 7. 类型体系与类型特化检索

**v1 的 4 类（fact/preference/episodic/procedural）几乎不起作用**——只影响淘汰容量与展示。

**v2 收敛到 3 类**（认知科学标准分法，preference 并入 semantic），并让类型**真正驱动行为**：

| 类型 | 内容 | 可变性 | 保留策略 | 检索加权 |
|---|---|---|---|---|
| **semantic** | 事实与偏好 | 可被矛盾 → SUPERSEDE | 长期、容量大 | 「是什么/我喜欢」类查询优先 |
| **episodic** | 带时间戳的事件 | **不可变**（发生过就是发生过） | 时间衰减、容量小 | 「上次/什么时候」类查询优先 + 时间衰减加权 |
| **procedural** | 技能/规则/怎么做 | 可被精化 → UPDATE | 最长期（技能不该被忘） | 「怎么办/怎么做」类查询优先 |

依据：MIRIX（3,457⭐，本仓库上游，多类型记忆）；旁证 TriMEM（预印本，**证明任何单一检索策略存在精度上界**，类型特化 +16.18% Recall@1）。

**对照实验**：类型特化检索 vs 单一策略基线，在同一基准上比 Recall@1——这是 Q3 的闸门。

**Q3 实测（2026-10-21，dog，bge-small INT8，`test_typed_retrieval`）**——收益是**条件性**的，两个基准给出对照：

| 基准 | 语料 | 基线 hit@1 | 类型特化 hit@1 | 提升 |
|---|---|---|---|---|
| v1 `typed_retrieval.json` | 24 条，跨类型话题互异 | 100%（18/18） | 100% | **+0（触顶）** |
| v2 `typed_retrieval_v2.json` | 16 条，episodic「近期 vs 久远」近重复对 | 68.8%（11/16） | 100%（16/16） | **+31.2pp** |

- **意图识别 100%**（两基准全对；关键词判据优先级 episodic > procedural > semantic）。
- **v1 触顶的原因**：真实查询都带意图词（「上次/怎么/多少度」），而这些词恰好和正确类型记忆的措辞相关，bge 纯余弦**已隐式路由对**——类型加权无用武之地。⇒ TriMEM 的 +16.18% 不是普适的，取决于基准难度。
- **v2 的提升来自 §7 设计的两个机制**：①**类型加权**修跨类型错（「上次充电在哪充的」基线误选 procedural 的充电流程）；②**episodic 时间衰减**修新近错（「上次保养换了什么」基线误选久远那条事件——纯余弦看不见时间戳，ts 不在 content 里）。提升全在 episodic（3/8→8/8），procedural/semantic 基线本就全对。
- **可复现性**：时间衰减的 recent/old 比值 = exp(gap/τ)，只依赖事件间隔、与运行的绝对时间无关，故结果稳定。
- **诚实边界**：类型特化不是免费午餐——只在「跨类型话题重叠 + 同话题多条事件需按新近消歧」时才显现价值；话题互异的简单语料上纯余弦已足够。

---

## 8. 治理标签

v1 只有一个 `sensitive` 布尔。v2 升级为三元标签（旁证 SAFE-Mem `2610.22959`、owner attribution `2610.25707`——写入时绑 owner 把跨用户凭证误用从 305 次配对实验的 92 次降到 1 次）：

- `owner`：这条记忆属于谁（user / agent / system）
- `scope`：流通范围（private / shared）
- `authority`：来源可信度（asserted / observed / inferred）

隐私路由规则不变：命中 `scope=private` → 强制端侧，原文不出设备。

---

## 9. 淘汰策略

- **分类型容量上限**（v1 已有）
- **强度衰减**：`strength` 随时间衰减，REINFORCE 时提升；淘汰按 `strength` 而非纯 FIFO（旁证 RSPM `2610.25561`：衰减未被强化的规则，存量 −30%）
- **episodic 优先淘汰**（事件会过时），**procedural 最后淘汰**（技能不该被忘）

---

## 10. 数据模型

新建 `memory` 表（`recall_memory` 的 schema 缺版本化/治理/溯源列，塞进 metadata JSON 会让查询变成 C++ 里解析 JSON）：

```sql
CREATE TABLE memory (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    type            TEXT NOT NULL,        -- semantic | episodic | procedural
    content         TEXT NOT NULL,
    embedding       BLOB,
    embed_model     TEXT,                 -- 模型版本 tag，换模型时检出失配
    strength        REAL DEFAULT 1.0,     -- REINFORCE 累加，衰减用
    access_count    INTEGER DEFAULT 0,
    -- 治理
    owner           TEXT DEFAULT 'user',
    scope           TEXT DEFAULT 'private',
    authority       TEXT DEFAULT 'asserted',
    -- 非破坏性版本化
    valid_from      TEXT,
    valid_until     TEXT,                 -- NULL = 当前有效
    superseded_by   INTEGER,
    supersedes      INTEGER,
    -- 溯源
    source_session  TEXT,
    source_turn     INTEGER,
    created_at      TEXT,
    updated_at      TEXT
);
CREATE INDEX idx_memory_type_valid ON memory(type, valid_until);
CREATE INDEX idx_memory_superseded ON memory(superseded_by);
```

---

## 11. API 设计

```cpp
enum class MemoryType { Semantic, Episodic, Procedural };
enum class MemoryAction { Created, Updated, Reinforced, Superseded, Noop };

struct CandidateFact { std::string text; MemoryType type; std::string owner, scope; };
struct IngestResult  { MemoryAction action; int id; float similarity; std::string reason; bool used_llm; };
struct Memory        { int id; MemoryType type; std::string content; float strength;
                       bool valid; int superseded_by; std::string valid_from, valid_until;
                       std::string owner, scope, authority; };

class EdgeMemoryStore {
  // ① 提取：对话轮 → 候选事实（受约束结构化输出）
  std::vector<CandidateFact> extract(const std::string& userTurn,
                                     const std::string& assistantTurn);
  // ② + ③ 消解 + 存储
  IngestResult ingest(const CandidateFact& fact);
  IngestResult ingestText(const std::string& text, MemoryType type);   // 跳过提取的直写入口

  // ④ 检索
  std::vector<MemoryHit> recall(const std::string& query, int topK = 4,
                                std::optional<MemoryType> filter = {},
                                bool includeHistory = false);

  // 显式 CRUD
  bool update(int id, const std::string& newContent);
  bool remove(int id);                       // 记录级删除，非 unlearning
  std::vector<Memory> list(std::optional<MemoryType> = {}, int limit = 0);
  std::vector<Memory> history(int id);       // 沿 supersedes 链回溯

  Stats stats() const;
};
```

**CLI**（demo 能被看见的前提）：

```
mirix-mem extract  --user "..." --assistant "..."     # 提取生成
mirix-mem ingest   "..." --type semantic
mirix-mem recall   "..." --top 4 [--type episodic] [--history]
mirix-mem update   <id> "..."
mirix-mem remove   <id>
mirix-mem list     [--type semantic] [--invalid]
mirix-mem history  <id>
mirix-mem stats
```

---

## 12. 评测方案

### 12.1 主基准：LongMemEval（MIT，1,163⭐）

数据：`xiaowu0162/longmemeval-cleaned`，三个变体：

| 变体 | 规模 | 用途 |
|---|---|---|
| `longmemeval_oracle.json` | 只含证据 session | 冒烟、最小可行 |
| `longmemeval_s.json` | ~115k tokens / ~40 sessions | **主评测** |
| `longmemeval_m.json` | ~500 sessions | 规模压力（可选） |

500 题，题型：`single-session-user` / `single-session-assistant` / `single-session-preference` / `temporal-reasoning` / **`knowledge-update`** / `multi-session`（`_abs` 后缀 = abstention）。五类能力：Information Extraction、Multi-Session Reasoning、**Knowledge Updates**、Temporal Reasoning、Abstention。

**关键：它自带记忆召回金标，不需要 LLM 就能评记忆系统**——
- `haystack_sessions` 里带证据的 turn 标了 `has_answer: true` → **turn 级召回准确率**
- `answer_session_ids` → **session 级召回准确率**

这与本项目「评测与生成解耦」的原则完全一致。`knowledge-update` 题型**直接考 SUPERSEDE 机制**。

**语言问题及解法**：LongMemEval 是英文，v1 用的是 `bge-small-zh`。已核实 `Xenova/bge-small-en-v1.5` 提供 `onnx/model_int8.onnx` + `tokenizer.json`，且是 **BERT/WordPiece 系**——**本项目自写的 C++ WordPiece 分词器直接可用**。所以：

> 同一个二进制、同一套 C++ 代码，**只改 config.ini 的模型路径**即可在「中文三域 demo」与「英文 LongMemEval」之间切换。这把 v1 的「换域只换语料」升级成**「换域 + 换语言 + 换模型都只换配置」**。

### 12.2 评测指标

| 层 | 指标 | 需要 LLM |
|---|---|---|
| 检索 | turn 级 / session 级 recall@k、MRR（LongMemEval 金标） | ❌ |
| 消解 | 五操作决策准确率（对照 FEB-30 口径；自建带标注场景集） | ❌ |
| 提取 | 候选事实的查全/查准 + **编造率**（用 `has_answer` 做金标） | 部分 |
| 端侧 | **System-1 覆盖率**、单次 ingest 延迟、RSS、DB 体积增长 | ❌ |
| 端到端 | QA 准确率（可选，需本地 4B） | ✅ |

### 12.3 规模控制

500 题 × ~40 sessions 全量 ingest 的耗时需先在 Orin 上实测；若超预算则**取分层子集**（按题型分层抽样），并**在结果里明确报告子集大小与抽样方式**——不得静默截断。

---

## 13. 特性 → 参考来源映射

| 特性 | 第一档依据（已落地） | 第二档旁证（预印本） |
|---|---|---|
| 两阶段管线（提取→消解） | **Mem0** 67.5k⭐ | — |
| 五操作生命周期 | **Mem0** ADD/UPDATE/DELETE/NOOP | FEB-30（reinforce/supersede 细分，有代码） |
| 非破坏性版本化 | **Graphiti** 32k⭐ 边失效 | Mem++ / TEM / PACMI |
| 记忆分层 | **Letta** 25k⭐ core/archival/recall | — |
| 类型体系 | **MIRIX** 3.4k⭐（本仓上游） | TriMEM（类型特化 +16.18%） |
| System-1/2 分层消解 | **Mem0** top-s 后决策 | Mnemon / MemFit |
| 治理标签 | — | SAFE-Mem / owner attribution / EP-Mem |
| 强度衰减淘汰 | — | RSPM / REM |
| 评测基准 | **LongMemEval** 1.1k⭐ MIT | FEB-30 / Bemula |
| 删除的诚实边界 | — | **Bemula**（有代码，无系统 >1.5%）/ MemCut |

---

## 14. 闸门

| 闸门 | 内容 | 判据（可验证） | 状态 |
|---|---|---|---|
| **Q1** | 新 `memory` 表 + `EdgeMemoryStore` 骨架 + 显式 `update`/`remove`/`list`/`history` | 每个操作有单元测试；版本化字段可回溯 | ✅ 41/41（Orin 实跑） |
| **Q2** | **五操作消解** + 三层判定 + 非破坏性 SUPERSEDE | 带标注消解场景集：五分类准确率 + **System-1 覆盖率** | ✅ 五分类 70%（可记四类 84%）、S1 覆盖 43%（Orin 实测，见 §5） |
| **Q3** | 类型特化检索（3 类各自策略） | 对照「单一策略」基线，Recall@1 是否提升 | ✅ 条件性提升：v1 触顶 +0 / v2 +31.2pp（episodic 3/8→8/8），见 §7 |
| **Q4** | **提取阶段**（受约束结构化输出） | 用 LongMemEval `has_answer` 金标测查全/查准 + **编造率** | ✅ 编造率 3.7%（上界）、查全 56.7%、JSON 解析 98.4%（见 §6） |
| **Q5** | **LongMemEval 接入**：英文模型（bge-small-en-v1.5 INT8）+ 数据加载 + turn/session 级召回评测 | 跑出 recall@k / MRR；**同一二进制只改 config 即切换中英** | — |
| **Q6** | CLI 八命令 + 在 Orin 录真实转录 → demo 页重建为**生命周期演示**（分步回放，非指标陈列） | 命令全跑通；线上 URL + Playwright 验证 | — |
| Q7（可选） | 治理标签三元组 + 强度衰减淘汰 | 看时间 | — |

**Q2 与 Q4 是技术风险所在**（本地 4B 的分类与提取准确率未知，已知它会编造）。Q5 是可信度跃升最大的一环（自证 GT → 公开基准）。

---

## 15. 诚实边界（必须写进 README 与 demo 页）

1. **`remove(id)` 是记录级删除，不是 unlearning。** Bemula 实测无系统遗忘请求端到端通过率 >1.5%；MemCut 实测 LLM 引导删除后仍 39–76% 可恢复。被删内容可能已进入摘要、consolidation 产物或模型上下文历史。**不得声称实现了「遗忘」。**
2. **LongMemEval 是英文，三域 demo 是中文**，两套评测**不可直接互相比较**；它们共同证明的是「换语言/换模型只改配置」。
3. **若只跑 LongMemEval 子集**，必须报告子集大小与分层抽样方式（Q4 用的是 oracle 变体、每题型取前 5 题共 30 题的确定性分层子集）。
4. **提取与消解准确率均已实测**：消解（Q2，见 §5）五分类 70%、可记四类 84%；提取（Q4，见 §6）查全 56.7%、编造率 3.7%（上界，人工核验多为改写非捏造）。约束成结构化输出**确实规避了 P2 的开放式编造**（提取 JSON 解析 98.4%、消解输出始终合法 JSON），但语义判断仍是短板（消解的 REINFORCE、提取漏最新值/隐式偏好）。
5. **类型特化检索的收益是条件性的**（Q3 已实测，见 §7）：话题互异的简单语料上纯余弦已触顶（v1 +0）；只有「跨类型话题重叠 + 同话题多条事件需按新近消歧」时才显现（v2 +31.2pp，全在 episodic）。TriMEM 的 +16.18% 是它自己数据上的结果，不能直接搬。
6. **第二档引用全部是无同行评审的预印本**，其中 6/8 无公开代码。
7. **单设备、未测热降频与功耗**；数字是冷启动/短负载下的。
8. **12 个未调用的 manager（2,146 行）**在 v2 中仍不使用——需在 README 明确标注为 legacy，不得再声称「复用 2,600 行」。

---

## 16. 风险

| 风险 | 等级 | 缓解 |
|---|---|---|
| 本地 4B 提取/消解准确率过低 | **高** | Q2/Q4 先小规模实测；不达标则退到 MemFit 式纯确定性路线，并如实报告 |
| LongMemEval 全量 ingest 在 Orin 上耗时过长 | 中 | 先实测单题耗时再定子集规模；分层抽样并报告 |
| 英文模型切换后分词器不匹配 | 中 | bge-small-en-v1.5 已核实是 WordPiece + 有 `tokenizer.json`；Q5 先跑分词对拍 |
| 新表与旧 `recall_memory` 并存造成混乱 | 中 | v2 只用 `memory` 表；旧表与 12 个 manager 标 legacy |
| demo 页承诺了代码里不存在的功能 | **高** | Q6 必须在 Q1–Q5 之后；demo 只录真实跑通的命令 |
