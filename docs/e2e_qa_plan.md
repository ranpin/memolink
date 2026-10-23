# 端到端 QA 评测方案（Q8）：memolink vs 朴素 RAG vs 无记忆

- **状态**：方案待拍板（判据预注册，未开工）
- **定位**：记忆系统评测金字塔的塔尖（设计文档 §12.2 列的「端到端 QA 准确率」），**不是 agent harness**
- **评测口径**：**与 LongMemEval 官方一致（GPT-4o judge → accuracy）**，保证可与公开方法对比
- **日期**：2026-10-23

---

## 0. 为什么做（补 Q1–Q7 的最后一个洞）

Q1–Q7 把**每个机制单独**证明能工作，但从没证明**这些机制合起来的系统，比一个更简单的替代方案更有用**。这是「我写了个正确的库」与「我做了个有价值的系统」的分水岭，也是面试里用人经理（「这到底 DO 什么」）和 ML 研究员（「五操作/版本化/类型特化这套复杂度值吗」）会同时戳的点。

Q8 用 LongMemEval 做端到端 QA：**有了 memolink 的记忆，LLM 答题是否比朴素 RAG / 无记忆更好，系统全程离线在 Orin。**

> 这是记忆系统的**标准价值证明方法**——LongMemEval、Mem0 的 LOCOMO 评测本身就是 QA 形态。dog 上链路已非正式跑通（本地 qwen3-4b + 检索记忆，「记忆增强 prompt、回答正确召回记忆事实、0.97s」），本方案只是把它形式化成带 baseline、带官方打分的 benchmark。

---

## 1. 三臂定义（干净消融：只变「记忆组织 + 检索方式」）

三臂**共用**：同一答题 LLM（本地 qwen3-4b / llama-server）、同一答题 prompt 模板（只换填入的记忆/上下文）、同一解码参数（`/no_think`、固定 temperature）、**同一 embedder（memolink 的 bge-small）**。唯一变量是「对话被组织成什么可检索单元 + 怎么检索」。

| 臂 | 记忆组织 | 检索 | 喂给答题 LLM 的上下文 |
|---|---|---|---|
| **A. memolink-full** | extract 候选事实 → 五操作消解/版本化 → `memory` 表（Q1/Q2/Q4） | 类型特化 recall（Q3） | top-k 条记忆 |
| **B. naive-rag** | 不提取/不消解/不版本化：haystack 逐 **turn** 作为检索单元 | embed 每个 turn + top-k 余弦（即 **Q5 的 turn 级检索**） | top-k 个原始 turn |
| **C. no-memory** | — | — | 只有问题（下界） |

**为什么 B 用「逐 turn 检索」**：这正是 Q5 已测的 turn 级检索（recall@10=0.78）。所以 **A vs B 恰好隔离出「提取 + 消解 + 版本化 + 类型路由」在裸 turn 检索之上额外带来了什么**——最干净的消融，且 B 的检索质量已有 Q5 数据可参照。

**公平性约束**：

- top-k 对齐（k=10，与 Q5 的 recall@10 一致）。
- 三臂答题 LLM / prompt 模板 / 解码参数完全一致。
- **报告每臂平均上下文 token 数作为协变量**：A 的记忆是压缩过的事实、B 是原始 turn，压缩本身是记忆系统的价值之一——不强行补齐长度，但必须披露这个不对称。
- 每臂产出官方格式的 hypothesis JSONL（`question_id` + `hypothesis`），交同一 GPT-4o judge 打分。

---

## 2. 指标口径（与 LongMemEval 官方一致，保证可对比）

**主判据 = LongMemEval 官方协议：GPT-4o judge → 逐题正确性标签（`autoeval_label`）→ 平均（accuracy）。**

- 官方流程（已核实 `xiaowu0162/LongMemEval` README）：系统输出存成 JSONL，每行 `question_id` + `hypothesis`（生成的答案）；跑官方 `src/evaluation/evaluate_qa.py gpt-4o <hypothesis.jsonl> <data.json>`，每题得 `autoeval_label`，用 `print_qa_metrics.py` 汇总成 accuracy。
- **这是能跟 Mem0 / 论文 baseline 等公开数字同口径对比的唯一方式**——token F1/EM 不是官方口径，无法对比，故降为次要。
- **离线纯度不受影响**：被测系统 memolink（提取/消解/存储/检索 + 本地 qwen3-4b 生成答案）**全程离线在 Orin**；GPT-4o 只用于**事后打分**（离线分析步，在 Mac 上跑），不进系统链路。这与 Mem0 / LongMemEval 论文的评测方式一致，不损害「端侧离线」主张。
- **judge 与生成解耦**：judge 是外部的 GPT-4o，≠ 生成本地答案的 qwen3-4b，无自证循环（延续「评测与生成解耦」原则）。

**次判据（确定性、可离线复现、交叉核对 judge）：token-level F1 + exact match（EM）。** 不作对比口径，仅作 sanity 与 judge 异常时的兜底。

**分题型报告**（6 题型），重点看 `knowledge-update`。

**依赖**：官方 judge 需 **OpenAI API key（GPT-4o）** + 克隆 LongMemEval 的 `src/evaluation` 脚本；judge 在 Mac 上跑，输入是 dog 离线生成的 hypothesis JSONL。

---

## 3. 子集规模与抽样

- **与 Q4/Q5 同一定确定性分层子集：每题型前 5 题 = 30 题**（6 题型 × 5），保证与 Q4/Q5 可比。
- **主闸门用 s 变体（完整 haystack）**——真实：系统不知道哪些 session 相关，必须全量 ingest。
- **先跑冒烟（oracle 变体 30 题）**验证 harness 端到端跑通。**冒烟是环境验证，不是闸门数字，不作里程碑汇报。**
- **运行时预算**：s 变体每题 396–616 turns；Q4 实测提取 37.3s/题（oracle，22–46 turns），s 变体提取约一个量级 ≈ 5–10 min/题，30 题 **≈ 3–5 小时**（A 臂 ingest 主导）。**须在 dog 上后台长任务跑，且脱离 ssh 会话、可断点核验。**
- **子集 vs full-set 的可比性边界**：公开榜单数字多在 full-set（~500 题）上。我们跑 30 题分层子集，**绝对值不等于 full-set 榜单数字**；可比性体现在 ① 指标定义与官方一致（同 GPT-4o judge、同 accuracy 口径）② **A/B/C 三臂同子集同 judge 的相对比较**（这才是本实验重点）。报告里必须写明子集规模与抽样方式。

---

## 4. 闸门判据（从机制推导，不写死阈值）

遵循「期望值从数据/机制推导、不硬编码数字」，用**预注册期望 + 决策树**，不设单一拍脑袋的「必须赢 X%」硬阈值。

**预注册期望（各可从机制推出）：**

- **E1（评测有效性）**：C（无记忆）应**显著低于** A、B。若 C 与 A/B 接近，说明问题不依赖记忆、评测无效 → 先修评测。（相对判据，非写死绝对值）
- **E2（so-what）**：A 总体 accuracy **显著高于** C。若 A ≈ C，系统无端到端价值 → **硬失败**。
- **E3（机制主假设）**：在 **knowledge-update** 题型（n=5）上，**A ≥ B**。依据：knowledge-update 的答案随时间变过，B 会把新旧矛盾的 turn 一起召回、答成旧值；A 的 SUPERSEDE/版本化把旧值标失效、recall 返回最新值——这正是版本化机制设计出来要解决的。
- **E4（次要）**：其余题型 B 有竞争力（单 session 题裸 turn 检索不弱），A 不必全面赢。

**决策树（实测数字 → 结论）：**

- A 总体 > B，且 knowledge-update 上 A > B → **强阳性**（系统复杂度端到端兑现）。
- A 总体 ≈ B，但 knowledge-update 上 A > B（且案例分析证实 SUPERSEDE 取到最新值）→ **机制确认的阳性**（版本化在该赢的地方赢了；总体持平是诚实且可解释的结果）。
- A 总体 < B，且 knowledge-update 上 A ≤ B → **阴性结果**，如实报告、重新定位（如「memolink 的价值在治理/可回溯，不在 QA 总分」）。

**统计诚实**：每题型 n=5 太小，**不声称统计显著性**；knowledge-update 按「方向 + 逐题案例分析」报告。总体（n=30）accuracy（官方 GPT-4o judge）作为实测数字报告，并明确子集规模与抽样方式（延续 §15 诚实边界第 3 条）。

---

## 5. 复测与防侥幸纪律

- 任一臂数字**反直觉或单次**（如 A 险胜 B、某题型异常），按「反直觉测量须复测」：**换一个独立变量重跑**——换抽样的题（如取每题型第 6–10 题、或换一组），**不是原样重跑同一子集**。
- **LLM 答题非确定性**：固定 temperature/seed；若 QA 答案波动，报多次运行方差（与 Q2 的 System-2 非确定性同理，预先披露）。
- **judge 一致性**：GPT-4o judge 本身有少量随机性，official 口径下可接受；若某题判罚反直觉，核对 hypothesis 与 gold 原文。

---

## 6. 基础设施盘点（现成 / 需搭）

- **现成**：LongMemEval s/oracle 数据（dog `bench/data`，Q5 已接入）、本地 qwen3-4b（llama-server）、memolink 记忆系统本体（Q1–Q7）、bge-small embedder。
- **需搭**：
  - `bench/e2e_qa_eval.py`（仿 Q4 `extract_eval.py`）：载子集 → 三臂准备上下文 → 调本地 4B 生成 `hypothesis` → 输出官方 JSONL（`question_id` + `hypothesis`），每臂一份。
  - **A 臂批量 ingest 路径**：对每题完整 haystack 做 extract + ingest（CLI 逐条太慢，需批量调用或薄脚本封装）。
  - **B 臂 baseline**：逐 turn embed（**复用 memolink embedder**）+ top-k 余弦（保证与 A 同 embedder）。
  - **官方打分**：克隆 `xiaowu0162/LongMemEval` 的 `src/evaluation`，在 **Mac** 上跑 `evaluate_qa.py gpt-4o`（需 **OpenAI API key**）；顺带算 token F1/EM 作次判据。
- 这些是实现细节，**闸门判据不依赖具体实现**。

---

## 7. 诚实边界（预先披露，不等面试官挖）

- 这是 LongMemEval（英文、s 变体）上的端到端 QA，与中文三域 demo **不可直接互比**（延续 §15 第 2 条）。
- n=30（每题型 5）是确定性分层子集，**必须报告子集大小与抽样方式**；绝对值不等于 full-set 榜单数字（§3 已说明）。
- **GPT-4o judge 是云端、仅用于事后打分**：被测系统 memolink 全程离线，打分用云 judge 不损害「端侧离线」主张，但需在 README/文档里写清这条口径。
- **若 memolink 总体不赢，如实报告**——本实验的价值在于回答「复杂度是否换来端到端收益」，阴性结果同样是必须知道的结论，防止简历过度声称。

---

## 8. 交付物

- 数字表：三臂 × 6 题型 × **官方 accuracy（GPT-4o judge）**，辅 token F1/EM；重点 knowledge-update。列头写清模型全名（答题=qwen3-4b、judge=gpt-4o、embed=bge-small-en-v1.5）。
- knowledge-update 案例分析（SUPERSEDE 是否取到最新值、B 是否答成旧值）。
- 结果写入 docs（设计文档 §14 闸门表加 Q8 行）+ README 核心结果表加 Q8。
- 若阳性：成为简历的「so-what」句——「离线 Orin 上，memolink 在 knowledge-update 题上比朴素 RAG 的 QA accuracy 高 X pp（LongMemEval 官方 GPT-4o judge 口径）」。
