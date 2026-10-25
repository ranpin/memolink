# 多模态记忆方案（Q9）：方案 A caption-then-embed + authority 感知消解

- **状态**：机制已实现并真机验证（commit `cf46a62`，`test_multimodal_authority` 36/36 + M3 回归全绿）；**Q9 闸门未整体通过**——M2 端到端与 M4a/M4b 完整场景集待做，详见 §8
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

**「拦下」的确切语义（用户 2026-10-25 拍板）**：asserted 候选撞上已有 observed 记忆时，**入库但不建取代关系**——两条并存，observed 仍是当前有效值，asserted 作为一条独立的低可信度记忆留下（不丢弃）。理由：符合 resolver 既有回退原则「CREATE，绝不覆盖或丢失旧记忆」，且不丢信息。「被拦」这件事要在 `ResolveDecision.reason` 里记明，demo 可展示。

**触发条件的硬约束**：规则**只在 authority 不同时介入**。这是 M3（不回归）的设计保证——纯文本对话里所有记忆都是 asserted，规则永不触发，Q2 的 30 场景消解结果必须逐条不变。

**落点**：`MemoryResolver::resolve` 需要拿到候选的 authority（现签名只有 candidate+type）。推荐把 authority 加进 `resolve` 参数，让决策逻辑集中在 resolver；matched 记忆的 authority 已可从 `Memory.authority`（`MemoryStore.h:38`）读到，不需额外查询。

**明确不在 Q9 范围**：「驾驶员显式纠正摄像头」（如「不对，我停的是 B1」）应允许 asserted 覆盖 observed——这需要 System-2 额外判「是否显式纠正」，而 4B 模型这个判断本身不可靠。Q9 先只做单向规则 + 拦下，显式纠正作为 Q9 之后的细化。

### 1.5 VLM 部署（step 0 已实测，2026-10-25）

**⚠️ 先纠正一个前提：dog 上的 qwen3-4b 不是多模态模型，必须另下 VLM。** 三条独立证据：

- gguf `general.architecture` = **`qwen3`**（纯文本架构；多模态会是 `qwen3vl` / `qwen2.5vl` / `qwen3omni`）
- llama-server `/props` 明确报 **`"modalities":{"vision":false,"video":false,"audio":false}`**
- 全机 `find -iname "*mmproj*"` **零命中**，且运行中的 server 命令行没有 `--mmproj`（llama.cpp 的视觉能力必须靠独立 mmproj gguf）

命名易混：**Qwen3-4B**（纯文本 dense）≠ **Qwen3-Omni**（多模态，但只有 30B-A3B MoE，无 4B 变体）≠ **Qwen3-VL-4B**（存在，是另一个模型）。

**🔴 顺带查出的披露问题：这个 qwen3-4b 是微调 checkpoint，不是 stock instruct。**

- 路径 `~/chenrunbin/qwen-trajectory-prediction/qwen3-4b-q4_k_m.gguf`，gguf `general.name` = **"Checkpoint 500 Merged"**，同目录还有 `.bak` / `.world.gguf` / `server.log`。
- ⇒ **Q2 五分类 70%、Q4 编造率 3.7%、座舱 demo 的全部回复，都是这个轨迹预测微调 checkpoint 产出的**，不是通用 instruct 模型。数字本身有效（实测就是它跑的），但**对外披露模型身份时必须写准**，不能写「Qwen3-4B-Instruct」。这也可能解释提取器为何偏爱叙述句。

**step 0 结论：网络通、盘够、内存够，可以下 VLM。**

| 项 | 实测 | 结论 |
|---|---|---|
| huggingface.co | `000` / FAIL | 直连被墙 |
| **hf-mirror.com** | **`200`** | ✅ 可用旁路 |
| modelscope.cn | `302` | ✅ 可用备选 |
| 磁盘 | 916G total / **507G avail** | ✅ 够 |
| 内存 | 29Gi total / **17Gi available** | ✅ 够（qwen3-4b 常驻 RSS 8.7GB） |
| llama.cpp build | `version: 1 (f3e1828)`，aarch64 | ✅ **已运行时验证支持 `qwen3vl`**（`/props` 报 `vision:True`），不用回退也不用重编 |

**VLM 已部署并跑通（2026-10-25 实测，不再是选型待定）**：

- 权重在 dog `~/chenrunbin/vlm/`：`Qwen3VL-4B-Instruct-Q4_K_M.gguf`（2,497,281,664 B）+ `mmproj-Qwen3VL-4B-Instruct-F16.gguf`（836,180,256 B），**均逐字节核对过 hf-mirror tree API 的 size**；gguf `general.architecture` = `qwen3vl`。
- 起服务：`llama-server -m <gguf> --mmproj <mmproj> --host 127.0.0.1 --port 8098 -ngl 99 -c 4096`，**~10s 就绪**；`/props` 报 `modalities:{vision:True, video:True, audio:False}`。
- 与 qwen3-4b（:8099）**两实例共存**，加载后 `free -h` 仍 **12Gi available**（VLM 占约 5GB）。
- **下载坑**：主模型一次成功（3.9 MB/s，~10 min），**mmproj 首次 wget RC=4 网络失败、0 字节**，重试即成 ⇒ 多文件下载必须逐个核对字节数，别只看退出码。
- 回退方案（`Qwen2.5-VL-3B-Instruct` GGUF）**已不需要**——build f3e1828 原生认 qwen3vl。

### 1.6 caption 实测基线（2026-10-25，合成图，已知真值）

用 PIL 生成**已知真值**的合成停车位图（蓝牌「B2」+「234」、红色圆角矩形、两个黑圆、灰背景），跑 4 档分辨率 × {定向提问 ×2, 开放描述 ×1}，temperature=0（同档两次结果完全一致）。

| 分辨率 | prompt_tok | 定向提问读数 | 开放描述读数 | 首帧延迟(定向/开放) |
|---|---|---|---|---|
| **640×480** | 336 | **B2 / 234 ✅** | B2 + 234 ✅ | **1.1s / 2.7s** |
| 960×720 | 726 | B2 / **34 ❌** | B2 + 34 ❌ | 1.3s / 2.9s |
| 1280×960 | 1236 | B2 / **23 ❌** | **「32」/「23」❌❌** | 2.0s / 4.1s |
| **1920×1440** | 2736 | **B2 / 234 ✅** | B2 + 234 ✅ | 5.9s / 7.7s |

**四条结论（均已复现，非单次测量）**：

1. **🔴 分辨率对 OCR 的影响是非单调的**——两端（640×480、1920×1440）全对，中间两档（960×720、1280×960）掉数字。**不是「越高越好」也不是「越高越差」**。机制未隔离（疑为视觉编码器在特定缩放档位的重采样伪影），不硬解释。⇒ **实践结论：分辨率必须钉死并逐档验证，不能默认取高。**
2. **定向提问 ≫ 开放描述**：定向「标志上有哪些文字和数字？逐个读出」→ 7 tokens / 0.8s / 全对；开放「描述图片内容」→ 59 tokens / 2.4s，且 1280×960 档开放描述连 B2 都读成「32」而定向仍对。⇒ **caption prompt 应结构化定向提问，不用开放式描述。**
3. **冷启动 23.0s，热态 1–3s；prompt cache 命中仅 0.2–0.3s**（llama-server 默认开 8192MiB prompt cache）。⇒ **报延迟必须区分冷/热态**，首次请求含预热。座舱新帧场景按热态首帧 1.1s（640×480 定向）预算。
4. **12 次跑零编造**：所有描述都在图内（蓝牌/白字/白边框/红矩形/两黑圆/灰背景），一个都没多。它甚至**拒绝从抽象几何形状推断语义**（说「红色矩形+两个黑圆」而不说「车」），符合「只描述可见、不推断」约束。

**⚠️ 合成图只验链路，结论不能外推到真实帧**：合成高对比文字是 OCR **最容易**的情况，它在 4 档里仍有 2 档读错 ⇒ 曾据此把「真实帧关键字段读取可靠性」列为 Q9 最大风险。**这个升级已被 §1.7 的真实帧实测推翻**，见下。

### 1.7 真实帧实测（2026-10-25，Wikimedia Commons 公开图，🔴 推翻 §1.6 的风险升级）

用 5 张**许可证明确可再分发**的 Commons 图（dog 连不上 Wikimedia，走 Mac 下载 → scp）。**真值来自 Commons 人工元数据（文件名 + ImageDescription），独立于被测 VLM**。temperature=0，两张文字牌做 3 档分辨率 × {定向, 开放} 重扫。

| 图 | 真值（Commons 元数据） | 许可证 / 作者 | 定向提问读数 | 结果 |
|---|---|---|---|---|
| sign_level2a.jpg | **Level 2A** | CC BY-SA 4.0 / Ser Amantio di Nicolao | 640→「2 A」、1280→「2 A」、1920→「2 A」 | **6/6 全对** |
| sign_stairb_level3.jpg | **Stair B / Level 3** | CC BY-SA 4.0 / Xnatedawgx | 三档均「Stair B / Level 3 / EXIT DISCHARGE / LEVEL 1 / STAIR」 | **6/6 全对**（连牌上附加文字都读出） |
| dash_parkingbrake.jpg | 琥珀色驻车制动警告灯 + 周围车控 | **CC BY 2.0** / Shixart1985 | —（开放描述） | 「黄色发光图标、圆圈内含 P + 类安全带符号」✅ 与真值一致 |
| seat_rearfacing.jpg | 后向式婴儿安全座椅 | CC BY-SA 3.0 / Pieter Kuiper | —（开放描述） | 「婴儿 + 汽车安全座椅」✅；但**漏掉「后向式」**，且读出标签「TAKTRA」（疑为 TAKATA 误读，**无法核实**） |
| parkingspace_cc0.jpg | I-10 休息区停车位 | **CC0** / DanTD | —（开放描述） | 「白色停车线 + 休息区场景」✅ 与真值一致 |

**🔴 核心发现：真实帧上关键字段 12/12 全对，M4b 风险远低于 §1.6 的估计。**

**干净的变量隔离**：合成图在 **1280×960** 把「234」读成「23」（❌），而真实牌子在**完全相同的 1280×960** 把「2A」读对（✅）。同分辨率、只换内容 ⇒ **§1.6 的失败是合成渲染的内容特异伪影，不是普遍的分辨率效应**。我据此做的「M4b 是 Q9 最大风险」升级**不成立，予以撤回**。

**其余实测**：

- **M4a 编造**：5 张图**未见明确编造**，描述都与 Commons 真值一致。但**受真值粒度限制**——Commons 描述只有一行，场景类图（仪表盘/座椅/停车位）里的细节断言（如「旋钮周围红色发光标识 0/P/D/R/N」「红色连体衣」「蓝牛仔裤腿部」）**无法逐条核实**。⇒ M4a 只能报「未见明确编造 + N 条不可核实细节」，不能报精确编造率。
- **延迟与 token 随分辨率线性增长**（与合成图一致）：le=640 → 338 tok / 定向 1.1s / 开放 2.0–2.9s；le=1280 → 1238 tok / 1.9s / 3.2–5.7s；le=1920 → 2738 tok / 4.1–5.7s / 6.4–7.2s。竖构图（359×640）仅 258 tok。
- **定向提问仍显著更省**：真实牌子上定向 1.1–1.9s / 6–20 tok，开放 2.0–7.2s / 60–110 tok，且**定向读数更干净可直接解析**。

**诚实边界（本节的）**：

- **n=2 张文字牌、12 次跑**，样本小；两张都是**大字号、高对比、光照良好**的容易案例。**小字号 / 逆光 / 运动模糊 / 脏污牌面未测**——这些才是真实座舱帧的难点。
- 真值来自 Commons 人工元数据，**不是我逐像素核验的**（当前模型不支持图像输入，`read_file` 看图失败）。文字牌的真值精确（文件名直接写明「Level 2A」「Stair B Level 3」），场景图的真值粗。
- 「TAKTRA」是否误读**无法判定**——需要能看图的人或更细的真值。

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

### M4 · caption 可靠性（**拆成两类失败模式**，多模态特有的新风险）

§1.6 实测发现这两类必须分开统计——它们是不同性质的失败，混在一个「编造率」里会掩盖真问题：

- **M4a · 编造率**：VLM 说了帧里**没有**的实体/属性。人工逐帧核验，报上界（口径同 Q4 提取编造率 3.7%）。
  - §1.6 合成图 12 次跑 **0 编造**——受约束 prompt（「只描述确实存在的、不要推测」）有效，且模型会拒绝从抽象形状推断语义。
- **M4b · 关键字段误读率**：帧里**有**、但读错了（如「234」读成「34」/「23」、「B2」读成「32」）。这是 §1.6 实测暴露的**真实主要风险**——合成高对比文字是 OCR 最容易的情况，4 档分辨率里仍有 2 档读错。
  - **必须在真实帧上测**，合成图结论不能外推。
  - 关键字段 = 场景依赖的、下游要用的字段（停车位场景就是楼层号 + 车位号）。

> 为什么 M4b 比 M4a 更值得担心：编造会被「只描述可见」约束压住（已验证），但**误读是静默的**——VLM 自信地输出「34」，下游 extract→ingest 会把错的值当成 observed 写进库，而 observed 在 authority 优先级里**高于** asserted。⇒ **一个误读会以最高可信度污染记忆库，且不会被 M2 的规则拦住。** 这是 Q9 最需要防的失效模式。

**缓解方向（M4b 若不过）**：定向提问已证比开放描述准（§1.6 结论 2）；可再加「读不准就说读不清、不要猜」的显式弃答约束；或对关键字段做二次确认提问。

### 预注册期望 + 决策树

- **E1（机制生效）**：M2 上 authority 感知版正确率应**显著高于** authority-blind 基线。若两者接近 → 规则没生效或场景构造有问题，**先修评测再继续**。
- **E2（非对称核心）**：「asserted 不得静默覆盖 observed」的场景应**全部拦住**。这是规则的非对称要点，漏一个就是机制没实现到位。
- **E3（不回归）**：M3 的 System-1 场景逐条不变；System-2 波动在 Q2 既有量级内。
- **E4a（不编造）**：M4a 编造率在可接受范围（与 Q4 的 3.7% 上界同量级或更低）。§1.6 合成图已给 0 编造的初步证据，真实帧复测。
- **E4b（关键字段读得对）**：M4b 误读率足够低，使「observed 写入的值可信」。这是比 E4a 更硬的门槛——因为误读会以最高可信度（observed）静默污染库。**若 M4b 不过，Q9 不能算通过**，哪怕 M2 全对。

**决策树**：

- M1 过 + E1/E2/E3/E4a/E4b 全过 → **Q9 通过**，authority 从过滤升级为决策成立。
- **E4b 不过（关键字段误读率高）→ Q9 不通过**，哪怕 M2 全对。先按 §1.6 结论调：钉死一个验证过的分辨率 + 定向提问 + 「读不准就说读不清」弃答约束，重测；仍不过则如实报告「observed 来源在当前 VLM 下不可信」，降级该场景。
- M2 过但 E4a 不过（编造率高）→ 收紧 caption prompt（更严的「只描述可见」约束）或降级为「只取高置信描述」，如实报告后重测 M4a。
- M3 回归（System-1 场景变了）→ **硬失败**，说明 authority 规则的触发条件写宽了，必须收窄到「authority 不同」才介入。
- M2 相对基线无提升 → 阴性结果，如实报告并重新定位（可能 authority 在座舱场景里本就不是有效信号）。

### 统计诚实

- 场景集 n 小（手写），**报 n、不声称统计显著性**，按「方向 + 逐例分析」报告（延续 Q8 纪律）。
- VLM caption 与 System-2 消解均有非确定性：固定 temperature；波动按 Q2 口径预先披露。
- 任一数字反直觉或单次 → 按「反直觉测量须复测」**换一个独立变量重跑**（换场景帧 / 换一组场景），不是原样重跑。

---

## 3. 场景集设计（M2 / M4a / M4b 用）

- **规模**：手写 ~12–20 个冲突场景，报 n。
- **🔴 帧来源铁律**：**不用 gated 驾驶帧**。本仓是 public + Pages 托管，alpamayo-edge 的驾驶帧来自 gated + license:other 数据集，有再分发风险（见既有教训）。用**自拍 / 合成 / 明确可再分发**的帧。
- **🔴 M4b 必须用真实照片，合成图只验链路（§1.7 已证实这条纪律本身是对的）**：§1.6 合成图在 4 档分辨率里 2 档读错，我曾据此把 M4b 升为「最高风险」；**§1.7 用真实帧重测后关键字段 12/12 全对，该升级已撤回**。⇒ 合成图上的失败**不能外推**到真实帧。
- **分辨率**：§1.7 真实帧在 640 / 1280 / 1920 三档长边**全对**；§1.6 的非单调失败已定性为**合成渲染的内容特异伪影**（同 1280×960 下合成「234」→「23」错、真实「2A」对）。⇒ 真实帧可优先取 **640 长边**（最快：定向 1.1s / 338 tok），但**难例（小字号 / 逆光 / 运动模糊 / 脏污牌面）仍需逐档验证**。
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

**执行 step 0：✅ 已完成（2026-10-25）**——dog 直连 huggingface.co 被墙（000），但 **hf-mirror.com 通（200）**、modelscope 通（302）；磁盘 507G 可用、内存 17Gi 可用；hf-mirror API 实查到官方 `Qwen/Qwen3-VL-4B-Instruct-GGUF`（488k 下载）可下。**Q9 不再阻塞。** 详见 §1.5。

**step 0 附带查出两件事**（已写进 §1.5）：① dog 的 qwen3-4b **不是多模态**（arch=`qwen3`、`modalities.vision=false`、无 mmproj），必须另下 VLM；② 它是**轨迹预测微调 checkpoint**（`general.name`="Checkpoint 500 Merged"），不是 stock instruct——影响对外模型身份披露口径。

**现成**：

- ✅ **VLM 已部署跑通**：`~/chenrunbin/vlm/` 的 Qwen3-VL-4B Q4_K_M + F16 mmproj（逐字节核对），:8098 实例 `/props` 报 `vision:True`，与 qwen3-4b（:8099）共存后仍 12Gi available（详见 §1.5）
- ✅ **caption 链路已验证**：合成图上跑通，冷启 23s / 热态 1–3s / cache 命中 0.2s，0 编造（详见 §1.6）
- bge-small embedder、完整 extract / 消解 / ingest 管线、cockpit app、governance（含 strict）、Q1–Q7 全部测试

**需搭**：

- `source_image` 列 + 幂等迁移
- `MemoryResolver` authority 感知规则（resolve 收候选 authority + 优先级裁决）
- **caption prompt 定型**——这里有一个 §1.6 暴露的**待解张力**：
  - §1.6 证明**定向提问**（「标志上有哪些文字数字？逐个读出」）比开放描述**更准且快 3 倍**（7 tok/0.8s vs 59 tok/2.4s）
  - 但座舱 demo 已证明**提取器对裸事实句会 NOOP**，caption 必须是**完整叙述句**才能被 extract 接住
  - ⇒ 定向提问的极短输出（「B2 / 234」）**喂不进提取器**。需折中：**定向提问拿准字段值 → 再模板化成叙述句**（如「车辆停在 B2 层 234 号车位」），或分两段 prompt。这个折中要在实现时定并验证。
- **真实场景帧集**（合规来源，非 gated）+ GT 标注 + **真实帧上重扫分辨率**（§1.6 的合成图结论不能外推）
- Q9 测试 harness（`test/test_multimodal_authority.cpp` 或 bench 脚本）
- demo 页多模态轮次（展示版本链 + 帧附件）

---

## 5. 诚实边界（预先披露，不等面试官挖）

1. **caption-then-embed ≠ 跨模态检索**。embedding 仍是文本空间，**不能声称「multimodal retrieval」或「以图搜图」**。准确表述是「视觉接地的文本记忆」（vision-grounded text memory）。
2. **VLM 是外设/配角，不是贡献点**——与 LLM 生成同等地位。Q9 的贡献点是 **authority 感知消解**这一记忆机制，不是「在 Orin 上跑了个 VLM」。
3. **图像附件 ≠ unlearning**；private 图像在 SQLite 里仍是明文（同现有治理边界：访问控制 ≠ 加密）。
4. **帧来源合规**：不用 gated 帧；仓库 public + Pages 托管。
5. **VLM caption 有两类失败，必须分开报**：**编造**（说了图里没有的）与**误读**（图里有但读错）。实测：受约束 prompt 下**编造为 0**（§1.6 合成 12 次 + §1.7 真实 5 张均未见明确编造）；**误读在合成图上真实存在**（4 档里 2 档读错），但**真实帧上关键字段 12/12 全对**（§1.7）。**误读仍比编造更危险**——它会以 observed 最高可信度静默写库、且不被 M2 规则拦。
6. **合成图上的 caption 结论不能外推到真实帧（§1.7 已实证这条纪律）**：§1.6 合成图的 OCR 失败在真实帧上**没有复现**（同 1280×960 下合成「234」→「23」错、真实「2A」对）。⇒ 合成图只能验链路，**任何能力/风险结论都必须在真实帧上重测**。
7. **🔴 §1.7 的真值粒度与样本限制**：真值来自 Commons 人工元数据（文件名 + 一行 ImageDescription），**不是我逐像素核验的**（当前模型不支持图像输入，`read_file` 看图失败）。文字牌真值精确（文件名直接写明「Level 2A」「Stair B Level 3」），场景图真值粗 ⇒ **M4a 只能报「未见明确编造 + N 条不可核实细节」，不能报精确编造率**。且 **n=2 张文字牌 / 12 次跑**，都是大字号、高对比、光照良好的容易案例，**小字号 / 逆光 / 运动模糊 / 脏污牌面未测**——这些才是真实座舱帧的难点。
8. **caption 延迟必须区分冷/热态**：§1.6 实测冷启动 23.0s、热态 1–3s、prompt cache 命中 0.2s；§1.7 真实帧热态定向 1.1–1.9s、开放 2.0–7.2s（随分辨率线性涨）。报数字要写清是哪一种。
9. **方案 A 不动 embedding 空间，Q1–Q7 数字有效**；若将来要「以图搜图」才考虑方案 B（需重标 System-1 阈值 + 新闸门，**不在 Q9 范围**）。
10. **单设备、未测热降频与功耗**（延续设计文档 §15 第 7 条）；两个 llama-server 常驻的内存/延迟数字是冷启动短负载下的。
11. **🔴 文本侧模型身份要写准**：dog 上跑提取/消解/回复的 qwen3-4b 实为 `qwen-trajectory-prediction` 的**微调 checkpoint**（gguf `general.name` = "Checkpoint 500 Merged"），**不是 stock Qwen3-4B-Instruct**。Q2/Q4/座舱 demo 的所有数字都由它产出——数字有效，但任何表格列头、README、简历里都不得写成「Qwen3-4B-Instruct」，应写「qwen3-4b-q4_k_m（Checkpoint 500 Merged 微调）」。

---

## 6. 风险

| 风险 | 等级 | 缓解 |
|---|---|---|
| **真值粒度不足 / 样本小（n=2 文字牌）** | **高** | Commons 一行描述不足以逐条核实场景图细节；当前模型不支持图像输入、我无法逐像素核验 ⇒ 需人工复核或更细 GT 才能报精确 M4a/M4b。难例（小字号 / 逆光 / 运动模糊 / 脏污牌面）未测 |
| 真实帧上关键字段误读（M4b） | 中（**从「最高」下调**） | §1.7 真实帧 **12/12 全对**、§1.6 的失败已定性为合成伪影 ⇒ 下调。但误读是**静默**的、会以 observed 最高可信度写库、不被 M2 规则拦 ⇒ 仍需定向提问 + 「读不清就说读不清」弃答约束 |
| VLM caption 编造（M4a） | 中 | 受约束 prompt 已验证有效（§1.6 合成 12 次 + §1.7 真实 5 张均 **0 明确编造**）；但真值粒度限制使**精确编造率测不出** |
| caption 文体不被提取器接住（裸事实句 → NOOP） | 中 | caption prompt 强制输出完整叙述句（座舱 demo 已验证的教训）；但 §1.6/§1.7 定向提问输出极短（6–20 tokens），**与「叙述句」要求冲突，需折中设计** |
| authority 规则误伤 Q2 | 中 | 规则只在 authority 不同时触发；M3 回归验证（System-1 逐条不变） |
| ~~分辨率对 OCR 的影响非单调~~ | ~~高~~ | **✅ §1.7 已定性**：合成渲染的内容特异伪影（同 1280×960 合成错、真实对），真实帧三档全对 ⇒ 真实帧优先取 **640 长边**（最快：定向 1.1s / 338 tok） |
| ~~dog 下不动 VLM 权重（网络受限）~~ | ~~高~~ | **✅ 已排除**：hf-mirror 200 通、权重已下并逐字节核对 |
| ~~llama.cpp build 不认 Qwen3-VL 的 mmproj~~ | ~~中~~ | **✅ 已排除**：`/props` 报 `vision:True`，原生支持 |
| ~~两 llama-server 实例内存不够~~ | ~~中~~ | **✅ 已排除**：两实例共存后仍 12Gi available |
| resolve 加 authority 参数牵动调用方 | 低 | ingest 已有 authority 参数，透传即可；调用方少 |

---

## 7. 交付物

- **Q9 数字表**：M2 authority 消解正确率（感知 vs authority-blind 基线，before/after）+ **M4a 编造率上界 + M4b 关键字段误读率**（分开报，不合并）。列头写清模型全名（caption=Qwen3-VL-4B-Instruct Q4_K_M；消解/提取=qwen3-4b-q4_k_m「Checkpoint 500 Merged」微调；embed=bge-small-zh-v1.5 INT8），报 n、报分辨率与冷/热态延迟口径。
- **caption 实测基线表**：§1.6（合成）+ §1.7（真实帧）的分辨率 × 提问方式扫描结果，含误读逐例、**真值来源与许可证标注**。
- **帧清单（manifest）**：每张测试帧记录 Commons 原始 URL + 许可证 + 作者 + 真值来源，供复现与署名。§1.7 已用的 5 张：`sign_level2a`(CC BY-SA 4.0, Ser Amantio di Nicolao) / `sign_stairb_level3`(CC BY-SA 4.0, Xnatedawgx) / `dash_parkingbrake`(CC BY 2.0, Shixart1985) / `seat_rearfacing`(CC BY-SA 3.0, Pieter Kuiper) / `parkingspace_cc0`(CC0, DanTD)。**若把帧提交进仓库，必须附 ATTRIBUTION 文件**（CC BY/BY-SA 要求署名）。
- **冲突场景逐例分析**：observed 是否按规则胜出、asserted 是否被拦、版本链是否正确。
- **M3 回归证据**：Q2 30 场景重跑结果，System-1 逐条不变的核对。
- 设计文档 §14 闸门表加 **Q9 行**；README 核心结果加 Q9。
- demo 页多模态轮次 + 帧附件展示（延续既有 demo 纪律：变化项强视觉对比 + 图例 + 数值标口径）。
- 若 M2 阳性，成为机制句：**「authority 从只做检索期过滤升级为参与消解决策——摄像头观察（observed）与驾驶员口述（asserted）冲突时按可信度裁决，而非按相似度裁决。」**

---

## 8. 实现状态（2026-10-25，commit `cf46a62`，dog/Orin 真机验证）

### 8.1 已实现并验证

| 项 | 内容 | 验证 |
|---|---|---|
| schema | `memory` 表加 `source_image TEXT` + **幂等迁移**（`PRAGMA table_info` 检查后才 ALTER，旧库可升级、新库 no-op） | C1 |
| 存储路径 | `ingestText` / `supersede` 收 `sourceImage`；**`supersede` 加 `authorityOverride`**——observed 取代 asserted 时新记录写 observed，**不继承旧记录的 asserted**（原实现继承，会让 authority 优先级白做） | C2–C13 |
| authority 闸门 | `authorityRank`（observed=2 > asserted=1 > inferred=0，未知/空按 asserted）+ `applyAuthorityGate`（**刻意做成 public static 纯函数**） | A1–A6、B1–B13 |
| 接线 | `resolve(candidate, type, authority)` 在 System-1 REINFORCE 与 System-2 两条出口后过闸门；`ingest(..., sourceImage)` 透传；`MemorySystem` 门面同步 | D1–D4 |
| 测试 | `test/test_multimodal_authority.cpp`（CMake EDGE 块注册） | **36/36 PASS** |

**为什么闸门必须是纯函数**：无 LLM 时 System-2 回退 CREATE、`targetId=0`，闸门在 `resolve()` 里**根本不会触发**。做成 public static 纯函数才能离线确定性验证整张真值表——这是本实现最关键的可测性设计。

**闸门真值表（B1–B13，离线确定性）**：

- 同 authority → **完全不介入**（B1/B2/B3）——M3 的机制级保证
- 候选更高 → UPDATE **升级为 SUPERSEDE** + `authorityPromoted=true`（B5/B9）；本就是 SUPERSEDE 则不动（B4）
- 候选更低 → SUPERSEDE/UPDATE **降级为并存 CREATE** + `blockedByAuthority=true`、`targetId` 清零、`blockedTargetId` 记住原目标（B6/B7/B8）
- REINFORCE / NOOP / 无目标 / targetId 不匹配 → **不介入**（B10–B13，防误伤）

### 8.2 M3 不回归：✅ 已通过（dog 实跑）

| 闸门 | 本次 | 文档记录 | 结论 |
|---|---|---|---|
| Q1 `test_edge_memory_store` | **41/41** | 41/41 | 一致 |
| Q2 `test_memory_resolver` | **21/30 = 70%** | 21/30 = 70% | **一致**（CREATE 7/7、UPDATE 6/6、REINFORCE 3/6、SUPERSEDE 5/6、NOOP 0/5；System-1 覆盖 13/30=43.3%；目标定位 14/14） |
| Q3 `test_typed_retrieval` | **18/18 → 18/18** | 基线 18/18 → 类型特化 18/18 | 一致 |
| Q7 `test_memory_governance` | **27/27** | 27/27 | 一致 |

Q2 的 30 个场景**全部是 asserted vs asserted（同 authority）**，闸门按设计一次都没触发 ⇒ 五分类结果与文档记录逐条一致，**M3 成立**。（此前会话曾测到 20/30，属 System-2 本地 4B 非确定性波动；本次 21/30 与文档一致。）

### 8.3 尚未完成（**Q9 闸门未整体通过**）

- **M2 端到端**：需要冲突场景集 + 本地 LLM 让 System-2 产出 SUPERSEDE/UPDATE，闸门才会在 `resolve()` 里真实触发。**目前只验证了闸门纯函数逻辑（B）与存储路径（C），没有验证「System-2 → 闸门 → 落库」的端到端链路。**
- **M4a/M4b 完整场景集**：§1.7 只测了 2 张文字牌（12/12）+ 3 张场景图，未达方案要求的 ~12–20 个冲突场景；难例（小字号 / 逆光 / 运动模糊 / 脏污）未测。
- **座舱 app 多模态轮次**：VLM caption → extract → ingest(authority=observed) 的应用层接线未做，`cockpit_assistant` 还没有图像入口。
- **caption prompt 定型**：§4 记录的张力（定向提问输出 6–20 tok 太短、喂不进要叙述句的提取器）未解决。
- **demo 页多模态轮次 + 帧附件展示**。
- 设计文档 §14 闸门表加 Q9 行、README 核心结果加 Q9——**等 M2 端到端通过后再加，不提前声称**。
