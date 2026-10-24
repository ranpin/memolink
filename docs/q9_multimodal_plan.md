# 多模态记忆方案（Q9）：方案 A caption-then-embed + authority 感知消解

- **状态**：方案待拍板（判据预注册，未开工）
- **定位**：记忆系统层的**新能力**（承载应用仍是座舱助手）。**不是** agent harness，**不是**视觉管线
- **路线**：方案 A（caption-then-embed）——用户 2026-10-25 拍板
- **日期**：2026-10-25
- **与 Q8 的关系**：Q8（端到端 QA 评测）仍 parked，等 judge 模型方案；Q9 是能力轨，独立推进，两者不冲突

---

## 0. 为什么做（多模态对「记忆系统」的真正增量在哪）

多模态最容易被做成「接了个 VLM 的 demo」。要避免这一点，必须先想清楚它给**记忆系统**带来什么机制增量。

核实代码后的关键发现：**`authority` 这个维度目前是死的。**

- `memory` 表有 `authority TEXT DEFAULT 'asserted'`（`InitDB.cpp`），取值 asserted / observed / inferred（设计文档 §8）。
- 但它在整个读路径里**只出现在治理过滤**：`MemoryStore.cpp` 的 `if (gov.authority) sql += " AND authority = ?"`（searchByEmbedding + list 各一处）。
- **消解完全不看它**：`MemoryResolver::resolve(candidate, type)` 签名里没有 authority；`ingest(...)` 虽然收 authority，但只是原样透传给 store 落库。

摄像头观察天然是 `observed`，驾驶员口述天然是 `asserted`。所以多模态的真正机制增量是：

> **让 authority 从「只做检索期过滤」升级为「参与消解决策」——observed 与 asserted 冲突时，按可信度优先级裁决，而不是按相似度裁决。**

这是 Q1–Q7 都没有的新规则，grounded 在已有 schema（列早就在）、有研究锚点（SAFE-Mem `2610.22959` 的 kind/authority/scope），且**只有引入第二种记忆来源（视觉）才会被激活**——纯文本对话里所有记忆都是 asserted，这条规则永远不会触发。这就是多模态与 authority 必须一起做的原因。

---

## 1. 机制设计

### 1.1 数据流（方案 A）

```
图像帧 → VLM 受约束描述（只描述可见、不推断）
       → 描述文本走现有 extract → 消解 → ingest 管线（authority=observed, source_image=<帧路径>）
       → 帧本身作为附件留存（溯源 + demo 展示）
```

**embedding 空间零改动**：入库的仍是 bge-small 对 caption 文本的 384 维向量。这是方案 A 相对方案 B（双 CLIP 命名空间）的核心优势——**Q1–Q7 所有数字保持有效**，因为检索空间、System-1 阈值标定（θ_new=0.50 / θ_dup=0.999，在 bge-small 上标定）全都不动。

### 1.2 模型分工（关键：不要用 VLM 顶替 qwen3-4b）

- **VLM 只负责 caption**（图像 → 文本描述）。
- **文本侧仍用 qwen3-4b**：提取（Q4 调过 prompt）、消解（Q2 标定过阈值与五分类）、回复生成。

理由：Q2/Q4 的标定与 prompt 调参全部是在 qwen3-4b 上做的。若让 VLM 兼做文本侧，等于换了消解/提取模型，**Q2 五分类 70%、Q4 编造率 3.7% 这些数字全部失效**，要重标重测。分开跑则一个数字都不动。

### 1.3 schema 增量

`memory` 表加一列：

```sql
ALTER TABLE memory ADD COLUMN source_image TEXT;
```

- 对齐已有的 `source_session` / `source_turn` 溯源列语义（都是「这条记忆从哪来」）。
- **不塞 metadata JSON**——延续设计文档 §10 的决定（可查字段不进 JSON，否则查询变成 C++ 里解析 JSON）。
- 迁移需幂等（先查 `PRAGMA table_info(memory)` 再 ALTER），避免重复启动报错。

### 1.4 authority 感知消解（Q9 的核心机制新增）

**优先级**：`observed > asserted > inferred`

**规则（叠在现有五操作消解之上，只在候选与已有记忆 authority 不同时触发）**：

| 冲突双方 | 裁决 | 说明 |
|---|---|---|
| observed(新) vs asserted(旧) | **SUPERSEDE** | 观察取代口述，建版本链（旧值失效保留） |
| asserted(新) vs observed(旧) | **拦下，不静默覆盖** | 低可信度来源不得静默改写高可信度观察 |
| 同 authority | **走现有五操作消解，行为不变** | 保证 Q2 数字不回退 |

**触发条件的硬约束**：规则**只在 authority 不同时介入**。这是 M3（不回归）的设计保证——纯文本对话里所有记忆都是 asserted，规则永不触发，Q2 的 30 场景消解结果必须逐条不变。

**落点**：`MemoryResolver::resolve` 需要拿到候选的 authority（现签名只有 candidate+type）。推荐把 authority 加进 `resolve` 参数，让决策逻辑集中在 resolver；matched 记忆的 authority 已可从 `Memory.authority`（`MemoryStore.h:38`）读到，不需额外查询。

**明确不在 Q9 范围**：「驾驶员显式纠正摄像头」（如「不对，我停的是 B1」）应允许 asserted 覆盖 observed——这需要 System-2 额外判「是否显式纠正」，而 4B 模型这个判断本身不可靠。Q9 先只做单向规则 + 拦下，显式纠正作为 Q9 之后的细化。

### 1.5 VLM 部署（dog 已实测可行，2026-10-25）

- **llama-server 当前 build 已支持多模态**：`--help` 实测有 `--mmproj FILE` / `--mmproj-url` / `--mmproj-auto` / `--image-max-tokens`，**无需重新编译**。
- **内存余量够**：`free -h` 实测 29Gi total / 12Gi used / **17Gi available**（旧记忆里 system_agent 占 26.5GB 已过时，该进程现已不在运行）；12Gi used 的大头是常驻 qwen3-4b llama-server（RSS 8.7GB）。再塞一个 2–3B Q4 VLM（约 2–4GB）可行。
- **两实例分端口**：qwen3-4b 保持 :8099 不动，VLM 另起 :8098。
- **缺 VLM 权重**：dog 上现在只有 qwen3-4b + llama.cpp 的 vocab 测试 gguf，**没有任何 VLM**。需下 Qwen2.5-VL-3B-Instruct Q4_K_M gguf + mmproj（候选，尺寸待 §4 step 0 核实网络后定）。

---

## 2. 闸门判据（Q9，从机制推导，不写死阈值）

延续「期望值从数据/机制推导、不硬编码数字」+ 预注册期望 + 决策树。

### M1 · 链路跑通（功能性前置，**不是里程碑数字**）

VLM caption → extract → ingest(authority=observed, source_image) 在 dog 端到端跑通；`memolink list` / `history` 能看到 observed 记忆与帧路径。二元判据。

> 按「别把环境验证当里程碑」纪律：M1 只证明链路通，**不作为 Q9 的成果数字汇报**。Q9 的里程碑数字是 M2。

### M2 · authority 消解正确率（**Q9 主判据，会动的数字**）

在手写冲突场景集（n 报告）上测两条规则是否正确触发：

- observed vs asserted 冲突 → 是否 SUPERSEDE 且版本链正确
- asserted vs observed 冲突 → 是否被拦下（未静默覆盖）

**对照 = 现状 authority-blind 消解器**（同一场景集、同一模型、只关掉 authority 规则）。

**诚实说明这个对比的性质**：authority-blind 基线在这类场景上**按构造就会错**（它根本不看 authority），所以 M2 是「补上前 vs 补上后」的 before/after，**不是公平对抗**。真正的质量门槛在 M2 内部的「不误伤」+ M3 的不回归。

### M3 · 不破坏既有（回归，**硬闸门**）

同 authority（asserted vs asserted）路径行为与 Q2 一致：重跑 Q2 的 30 场景消解集，五分类结果与 System-1 覆盖率不回退。

- 允许 System-2 本地 4B 的非确定性波动（按 Q2 既有口径披露：文档 21/30、复测 20/30 属同量级波动）。
- **但 System-1 命中的场景必须逐条不变**（System-1 是确定性的，变了就是规则误伤）。

### M4 · caption 编造率（诚实，多模态特有的新风险）

VLM 对场景帧的描述是否编造帧里没有的实体/属性——人工逐帧核验，报**编造率上界**（口径同 Q4 提取编造率 3.7%）。

这是多模态引入的**新**风险：Q4 测的是文本提取的编造，VLM 看图说话是另一条路径，**不能默认它和文本提取一样干净**。

### 预注册期望 + 决策树

- **E1（机制生效）**：M2 上 authority 感知版正确率应**显著高于** authority-blind 基线。若两者接近 → 规则没生效或场景构造有问题，**先修评测再继续**。
- **E2（非对称核心）**：「asserted 不得静默覆盖 observed」的场景应**全部拦住**。这是规则的非对称要点，漏一个就是机制没实现到位。
- **E3（不回归）**：M3 的 System-1 场景逐条不变；System-2 波动在 Q2 既有量级内。
- **E4（caption 可用）**：M4 编造率在可接受范围（与 Q4 的 3.7% 上界同量级或更低）。

**决策树**：

- M1 过 + E1/E2/E3/E4 全过 → **Q9 通过**，authority 从过滤升级为决策成立。
- M2 过但 M4 编造率过高 → 收紧 caption prompt（更严的「只描述可见」约束）或降级为「只取高置信描述」，如实报告后重测 M4。
- M3 回归（System-1 场景变了）→ **硬失败**，说明 authority 规则的触发条件写宽了，必须收窄到「authority 不同」才介入。
- M2 相对基线无提升 → 阴性结果，如实报告并重新定位（可能 authority 在座舱场景里本就不是有效信号）。

### 统计诚实

- 场景集 n 小（手写），**报 n、不声称统计显著性**，按「方向 + 逐例分析」报告（延续 Q8 纪律）。
- VLM caption 与 System-2 消解均有非确定性：固定 temperature；波动按 Q2 口径预先披露。
- 任一数字反直觉或单次 → 按「反直觉测量须复测」**换一个独立变量重跑**（换场景帧 / 换一组场景），不是原样重跑。

---

## 3. 场景集设计（M2 / M4 用）

- **规模**：手写 ~12–20 个冲突场景，报 n。
- **🔴 帧来源铁律**：**不用 gated 驾驶帧**。本仓是 public + Pages 托管，alpamayo-edge 的驾驶帧来自 gated + license:other 数据集，有再分发风险（见既有教训）。用**自拍 / 合成 / 明确可再分发**的帧。
- **场景类型（座舱相关，每类都要有 GT 标注期望动作）**：

| 场景 | 冲突 | 期望动作 |
|---|---|---|
| 停车位 | 摄像头 observed「停在 B2 靠电梯口」vs 驾驶员先前 asserted「停在 B1」 | observed SUPERSEDE asserted，建版本链 |
| 反向覆盖 | 驾驶员 asserted「我停在 B1」试图改写已有 observed「B2」 | **拦下**，不静默覆盖 |
| 车内物品 | observed「后座装了儿童座椅」 | CREATE（authority=observed） |
| 仪表警告 | observed「胎压警告灯亮」 | CREATE，episodic |
| 乘客/隐私 | observed「副驾有人」 | CREATE + scope=private（与治理交互：其他驾驶员不可见） |
| 同 authority 对照 | asserted vs asserted 冲突 | 走现有五操作，**行为与 Q2 一致**（M3 用） |

- 每个场景标注：帧路径、期望 caption 要点、期望动作、期望 authority/scope、期望版本链。

---

## 4. 基础设施盘点（现成 / 需搭）

**执行 step 0（先做，决定后续可行性）**：核实 dog 能否下载 VLM 权重（网络可达性 / 是否需 hf-mirror 旁路 / 或本机下载再 scp）。**这一步不通，整个 Q9 阻塞**——先验证再动手。

**现成**：

- llama-server 多模态支持（dog 实测 `--mmproj`）、17Gi 内存余量（实测）
- bge-small embedder、完整 extract / 消解 / ingest 管线、cockpit app、governance（含 strict）、Q1–Q7 全部测试

**需搭**：

- VLM 权重（gguf + mmproj）下到 dog；第二个 llama-server 实例（:8098）
- `source_image` 列 + 幂等迁移
- `MemoryResolver` authority 感知规则（resolve 收候选 authority + 优先级裁决）
- 受约束 caption prompt（**只描述可见、不推断**；且要输出**完整叙述句**——座舱 demo 已验证提取器对裸事实句会 NOOP，caption 必须是叙述体才能被 extract 接住）
- 场景帧集（合规来源）+ GT 标注
- Q9 测试 harness（`test/test_multimodal_authority.cpp` 或 bench 脚本）
- demo 页多模态轮次（展示版本链 + 帧附件）

---

## 5. 诚实边界（预先披露，不等面试官挖）

1. **caption-then-embed ≠ 跨模态检索**。embedding 仍是文本空间，**不能声称「multimodal retrieval」或「以图搜图」**。准确表述是「视觉接地的文本记忆」（vision-grounded text memory）。
2. **VLM 是外设/配角，不是贡献点**——与 LLM 生成同等地位。Q9 的贡献点是 **authority 感知消解**这一记忆机制，不是「在 Orin 上跑了个 VLM」。
3. **图像附件 ≠ unlearning**；private 图像在 SQLite 里仍是明文（同现有治理边界：访问控制 ≠ 加密）。
4. **帧来源合规**：不用 gated 帧；仓库 public + Pages 托管。
5. **VLM caption 编造是新风险**，M4 单独测，不默认它和文本提取一样干净。
6. **方案 A 不动 embedding 空间，Q1–Q7 数字有效**；若将来要「以图搜图」才考虑方案 B（需重标 System-1 阈值 + 新闸门，**不在 Q9 范围**）。
7. **单设备、未测热降频与功耗**（延续设计文档 §15 第 7 条）；两个 llama-server 常驻的内存/延迟数字是冷启动短负载下的。

---

## 6. 风险

| 风险 | 等级 | 缓解 |
|---|---|---|
| dog 下不动 VLM 权重（网络受限） | **高** | step 0 先核实；hf-mirror 旁路或本机下载再 scp |
| VLM caption 编造 / 漏关键细节 | **高** | 受约束 prompt + M4 实测编造率；必要时降级为只取高置信描述 |
| caption 文体不被提取器接住（裸事实句 → NOOP） | 中 | caption prompt 强制输出完整叙述句（座舱 demo 已验证的教训） |
| 两 llama-server 实例内存不够 | 中 | 已测 17Gi 余量；不够则换更小 VLM 或串行换模型 |
| authority 规则误伤 Q2 | 中 | 规则只在 authority 不同时触发；M3 回归验证（System-1 逐条不变） |
| resolve 加 authority 参数牵动调用方 | 低 | ingest 已有 authority 参数，透传即可；调用方少 |

---

## 7. 交付物

- **Q9 数字表**：M2 authority 消解正确率（感知 vs authority-blind 基线，before/after）+ M4 caption 编造率上界。列头写清模型全名（caption=Qwen2.5-VL-3B-Instruct、消解/提取=qwen3-4b、embed=bge-small-zh-v1.5 INT8），报 n。
- **冲突场景逐例分析**：observed 是否按规则胜出、asserted 是否被拦、版本链是否正确。
- **M3 回归证据**：Q2 30 场景重跑结果，System-1 逐条不变的核对。
- 设计文档 §14 闸门表加 **Q9 行**；README 核心结果加 Q9。
- demo 页多模态轮次 + 帧附件展示（延续既有 demo 纪律：变化项强视觉对比 + 图例 + 数值标口径）。
- 若 M2 阳性，成为机制句：**「authority 从只做检索期过滤升级为参与消解决策——摄像头观察（observed）与驾驶员口述（asserted）冲突时按可信度裁决，而非按相似度裁决。」**
