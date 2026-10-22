# MIRIX-CPP — 端侧 Agent 记忆系统（Jetson Orin）

> 🎬 **在线演示**：<https://ranpin.github.io/mirix-cpp/> —— 真实 CLI 转录的**生命周期分步回放**（提取 → 消解 → 检索 → 矛盾更新 → 版本回溯 → 删除），非模拟、非指标陈列。

在 **Jetson Orin** 上离线跑通的**多类型 Agent 记忆系统**：记忆的**提取 → 消解 → 存储 → 检索 → 更新 → 删除 → 版本回溯**全程在本地完成，不依赖云端。C++ 实现——自写 WordPiece 分词器 + ONNX INT8 推理做本地 embedding，本地 4B LLM 做矛盾消解与事实提取。记忆分层架构参考自 [Mirix-AI/MIRIX](https://github.com/Mirix-AI/MIRIX)（Apache-2.0），**v2 记忆系统本体与端侧推理为本仓库新写**。

## 核心结果（Orin 实测，可复现）

| 闸门 | 内容 | 实测判据 |
|---|---|---|
| **Q1** | 版本化 CRUD（独立 `memory` 表 + 非破坏性版本化） | 41/41 单元测试 |
| **Q2** | 五操作消解（三层判定 + System-2 本地 4B） | 五分类 70%、可记四类 84%、目标定位 14/14 |
| **Q3** | 类型特化检索（意图路由 + 同类型加权 + 时间衰减） | 条件性提升：v2 基准 hit@1 **+31.2pp**（episodic 3/8→8/8） |
| **Q4** | 受约束提取（本地 4B，JSON 数组输出） | 编造率 **3.7%**（上界）、答案查全 56.7%、JSON 解析 98.4% |
| **Q5** | LongMemEval 接入 + 跨语言 config-only 切换 | s 变体 recall@10=0.78 / MRR=0.62；中英切换 5/5（dim 512↔384） |
| **Q6** | `mirix-mem` CLI 八命令 + 生命周期 demo | 八命令 Orin 全跑通；线上 Playwright 23/23 |

数字由仓库内程序在 Orin 上实跑生成（`bench/results/*.json`、各 `test_*` 评测程序），与 LLM 生成解耦。

## 记忆生命周期（demo 回放的就是这条链）

```
① 提取 (Q4)    对话轮 ─────────▶ 候选事实      （本地 4B，受约束 JSON 数组）
② 消解 (Q2)    候选事实 ───────▶ 五操作之一    （三层判定）
③ 存储 (Q1)    ───────────────▶ memory 表      （非破坏性版本化）
④ 检索 (Q3)    query ─────────▶ top-k          （类型特化：意图路由 + 加权 + 时间衰减）
⑤ CRUD (Q1)    update / remove / history       （显式、可回溯）
```

**五操作**：`CREATE` / `UPDATE` / `REINFORCE` / `SUPERSEDE` / `NOOP`——前四类取自 FEB-30（`2610.22209`，Amazon，有 benchmark 仓）的 create/update/reinforce/supersede 分类法，`NOOP` 取自 Mem0 的 ADD/UPDATE/DELETE/NOOP。

**三层消解**（Q2）：
- **System-1（确定性，不调模型）**：相似度 < θ_new(0.50) → CREATE；≥ θ_dup(0.999) → 逐字 REINFORCE。
- **System-2（模糊地带调本地 4B）**：约束输出四选一 {UPDATE, SUPERSEDE, REINFORCE, NOOP} + target_id。
- **回退**：LLM 不可用 / 解析失败 → CREATE（绝不覆盖或丢失旧记忆）。

**非破坏性版本化**（Q1）：SUPERSEDE 不删旧记忆，而是标记 `valid_until` + 建 `superseded_by`/`supersedes` 链，`history` 可回溯整条演化（Graphiti 式边失效的端侧落地）。

## 关键机制

**类型特化检索（Q3）**：关键词判查询意图（episodic > procedural > semantic）+ 同类型加权 ×1.6 + episodic 时间衰减 `exp(-age/365d)`。**收益是条件性的**——v1 基准（话题互异）纯余弦已触顶 +0，v2 基准（含「近期 vs 久远」近重复对）+31.2pp。

**受约束提取（Q4）**：本地 4B 输出受约束为 JSON 数组（每条 `{text, type, owner, scope}`），非自由文本——规避了开放式生成的编造。LongMemEval `has_answer` 金标实测编造率 3.7%（上界，多为改写非捏造）。

**跨语言 config-only 切换（Q5）**：同一份 C++ 二进制、零代码改动，只把 `config.ini` 的模型路径从 bge-small-zh（512 维）换成 bge-small-en-v1.5（384 维），自写 WordPiece 分词器 + OnnxEmbedder 直接可用（`test_edge_embedding` 5/5）——「记忆抽象语言无关」的物证。

## CLI：`mirix-mem` 八命令（Q6）

```bash
mirix-mem extract  --user "..." --assistant "..."   # ① 提取候选事实
mirix-mem ingest   "..." --type semantic            # ②③ 消解 + 存储
mirix-mem recall   "..." --top 4 [--type T]         # ④ 类型特化检索
mirix-mem update   <id> "..."                       # 显式更新
mirix-mem remove   <id>                             # 记录级删除
mirix-mem list     [--type T] [--invalid]           # 列举
mirix-mem history  <id>                             # 版本链回溯
mirix-mem stats                                     # 统计
```

## 端侧推理基座（本项目新写，v1/v2 共用）

| 组件 | 文件 | 实测 |
|---|---|---|
| C++ WordPiece 分词器 | `src/models/WordPieceTokenizer.{h,cpp}` | 与 HF `tokenizers` 逐 token 对拍 **25/25** 一致（中英混排 / 标点 / emoji / 日韩） |
| ONNX Runtime C++ 推理 | `src/models/OnnxEmbedder.{h,cpp}` | FP32 与 Python onnxruntime 余弦 **1.00000000**（max diff 2.9e-7）；CPU-only，GPU 整块留给 LLM |

**v2 记忆系统本体**：`EdgeMemoryStore`（独立 `memory` 表 + 版本化 CRUD）、`MemoryResolver`（三层消解）、`MemoryExtractor`（受约束提取）、`mirix-mem`（CLI）。复用 `StorageManager` + `InitDB` schema 与上述 embedding 基座；**不走上游 `recall_memory` 多 manager 路径**——v2 用独立 `memory` 表，避开 user/agent/session 外键约束（该约束曾让 v1 编排首跑 40/40 全失败）。

## 跨域 / 跨语言可扩展性

记忆抽象（3 类 semantic / episodic / procedural，源自认知科学分类）领域无关，换域只换语料 / 模型、不改代码：

- **跨语言**（v2，Q5）：config-only 中英切换；LongMemEval（公开英文基准，MIT）s 变体 recall@10=0.78。
- **跨领域**（早期 P4，同一 embedding 基座）：座舱 / 机器人 / 自动驾驶三域，同一二进制零改码，recall@4 ≥ 0.967。

## 诚实边界

1. **System-1 确定性覆盖仅 ~32%**（可记候选口径；全候选 43%），远低于设计假设的 70–80%——新颖度信号无法区分 UPDATE/SUPERSEDE/REINFORCE，矛盾是语义量，模糊地带必须交给 System-2 本地 LLM。
2. **类型特化检索的收益有条件**：v1 基准饱和（+0），v2 基准 +31.2pp——不是普适提升，取决于语料是否可分（TriMEM 报告的 +16.18% 不普适）。
3. **提取编造率 3.7% 是上界**，人工核验多为 assistant 建议的改写 / 温和推断，非凭空捏造。
4. **REINFORCE 是本地 4B 的系统性弱点**（等价重述 / 信息更少子集被误判 UPDATE）；NOOP 过滤主要归提取阶段（寒暄低相似度会落进 System-1 的 CREATE 快路径）。
5. **检索 / 消解评测与 LLM 生成解耦**：只评测记忆质量，不含生成质量评测。
6. **zh 基准语料与标注均手写**（GT 自定义）；LongMemEval 是公开英文基准。
7. **embedding 走 CPU**（onnxruntime 在 Orin 上 CPU-only），GPU 留给 LLM；单设备、未测热降频与功耗；INT8 是 onnx-community 预量化模型，非本项目量化。

## 构建与复现（Orin）

```bash
# 依赖：sqlite3 / curl / jsoncpp 开发头（apt）；onnxruntime aarch64 release
sudo apt install -y libsqlite3-dev libcurl4-openssl-dev libjsoncpp-dev

# config.ini 的 [edge] 段填本地 embedding 模型与分词器路径
#   embedding_model_path = .../model_quantized.onnx   (INT8)
#   tokenizer_path       = .../tokenizer.json
# config.ini 的 local_url 填本地 llama-server 地址（System-2 消解 + 提取用）
#   local_url = http://127.0.0.1:8099

mkdir build && cd build
cmake .. -DMIRIX_ENABLE_EDGE=ON
make -j8 mirix-mem

# 跑通生命周期
./mirix-mem ingest "用户喜欢深蓝色的车" --type semantic
./mirix-mem ingest "用户现在喜欢红色的车" --type semantic   # → SUPERSEDE
./mirix-mem recall "我的车是什么颜色偏好" --top 3          # → 返回红色
./mirix-mem history 1                                      # → 版本链

# 复现各闸门评测
make -j8 test_edge_memory_store test_memory_resolver test_typed_retrieval test_longmemeval_retrieval
```

## 桌面版（多模态采集 / GUI / API）

桌面功能（相机 / 音频 / 截屏采集、Qt GUI、HTTP API）通过 CMake 开关保留，默认关闭：

```bash
cmake .. -DMIRIX_ENABLE_CAPTURE=ON -DMIRIX_ENABLE_GUI=ON -DMIRIX_ENABLE_API=ON
```

> 端侧构建（默认）只链 sqlite3 + curl + jsoncpp + onnxruntime，不依赖 OpenCV / PortAudio / Qt5。桌面版定位与边界见 git 历史。

## 与 MIRIX 上游的关系

记忆分层架构（Core / Episodic / Semantic / Procedural / Resource / Knowledge Vault + Recall）参考自 [Mirix-AI/MIRIX](https://github.com/Mirix-AI/MIRIX)（Apache-2.0）。v2 收敛到 3 类（semantic / episodic / procedural），用独立 `memory` 表 + 版本化 CRUD 重建记忆系统本体；端侧推理（分词器 + ONNX）为本仓库新写。

## 文档

- **[v2 记忆系统设计](docs/memory_v2_design.md)** —— 权威方案 + Q1–Q7 闸门 + 实测结果
- [架构文档](docs/architecture.md) · [数据库设计](docs/database.md) · [记忆系统](docs/memory.md) · [测试说明](docs/testing.md)
