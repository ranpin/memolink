# MIRIX-CPP — 端侧 Agent 记忆系统（Jetson Orin）

> 🎬 **在线演示**：<https://ranpin.github.io/mirix-cpp/>

在 **Jetson Orin** 上离线跑通的 Agent 记忆系统：**写入 → 本地编码 → 跨类型检索 → 分类型有界淘汰**，全程不依赖云端。记忆分层架构参考自开源项目 [Mirix-AI/MIRIX](https://github.com/Mirix-AI/MIRIX)（Apache-2.0），本仓库在其 C++ 记忆存储层之上，新写了端侧推理与记忆编排。

## 核心结果（Orin 实测）

**三域检索评测**——同一个二进制、零代码改动，只换语料跑通三个域：

| 域 | 语料 | recall@4 | MRR | hit@1 |
|---|---|---|---|---|
| 座舱 cockpit | 40 条 | 0.9667 | 1.0000 | 15/15 |
| 机器人 robot | 25 条 | 1.0000 | 0.9667 | 14/15 |
| 自动驾驶 av | 25 条 | 1.0000 | 1.0000 | 15/15 |

**端侧推理对拍**：

- **C++ WordPiece 分词器**：与 HuggingFace `tokenizers` **逐 token 对拍 25/25 完全一致**（覆盖中英混排/标点/连续标点/emoji/日韩/带音调拉丁）。
- **ONNX Runtime C++ 推理**：FP32 与 Python onnxruntime **余弦 1.00000000**（max diff 2.9e-7），证明 C++ 实现正确。
- **INT8 vs FP32 top-k 一致性**：聚合指标（recall@k/MRR/hit@1）**完全相同**；top-k 尾部（rank 3-4 的无关项）会重排，但 expected id 的排名不变（hit@1 全同）⇒ **INT8 保住检索质量，只扰动无关尾部**。

## 端侧三件套（本项目新写）

| 组件 | 文件 | 说明 |
|---|---|---|
| C++ WordPiece 分词器 | `src/models/WordPieceTokenizer.{h,cpp}` | 自写 UTF-8 解码 + BertNormalizer + BertPreTokenizer + WordPiece 贪心，配置读自 `tokenizer.json`，不依赖 Rust `tokenizers` |
| ONNX Runtime C++ 推理 | `src/models/OnnxEmbedder.{h,cpp}` | onnxruntime 原生 C++ API（pImpl 隔离头），跑 bge-small-zh INT8，CPU-only（刻意把 GPU 留给 LLM） |
| 记忆编排器 | `src/database/managers/MemoryOrchestrator.{h,cpp}` | 写入/跨类型检索/敏感标记传播/实时写入/分类型 FIFO 淘汰 |

**复用的部分**：mirix-cpp 原有的 ~2,600 行记忆存储层（`recall_memory` 等表的 manager、`StorageManager`、`InitDB` schema）原样复用，未改动。

## 跨域可扩展性

三个域用**完全一致的类型体系**（fact / preference / episodic / procedural），只换语料、不改代码。这是「换域只换语料」的物证：

- 语料：`bench/corpus/{cockpit,robot,av}.json`
- 标注评测集：`bench/eval/{cockpit,robot,av}_eval.json`（每条查询带 `expected_ids`）
- 评测程序：`test/test_retrieval_eval.cpp`（算 recall@k / MRR / hit@1，与 LLM 生成解耦）
- INT8/FP32 top-k 对比：`bench/compare_topk.py`

## 诚实边界

- **语料与标注均由本人手写**，GT 是自定义的——证明的是「检索能按语义找回该找回的」，不是与公开基准对比。
- **检索评测与 LLM 生成解耦**：本项目只评测记忆检索质量，不含生成质量评测。
- **embedding 走 CPU**（onnxruntime 在 Orin 上是 CPU-only），未测 GPU embedding。
- **INT8 量化来自 onnx-community 的预量化模型**，非本项目量化。
- **`recall_memory` 有 user/agent/session 外键**，编排器用幂等默认实体行满足约束（不关外键）。

## 构建与复现（Orin）

```bash
# 依赖：sqlite3 / curl / jsoncpp 开发头（apt）；onnxruntime aarch64 release
sudo apt install -y libsqlite3-dev libcurl4-openssl-dev libjsoncpp-dev

# config.ini 的 [edge] 段填上模型与分词器路径
#   embedding_model_path = .../model_quantized.onnx   (INT8)
#   tokenizer_path       = .../tokenizer.json

mkdir build && cd build
cmake .. -DMIRIX_ENABLE_EDGE=ON
make -j8 test_retrieval_eval test_memory_orchestrator

# 三域检索评测
./test_retrieval_eval ../bench/corpus/cockpit.json ../bench/eval/cockpit_eval.json eval.db out.json 4
```

## 桌面版（多模态采集 / GUI / API）

桌面功能（相机/音频/截屏采集、Qt GUI、HTTP API）通过 CMake 开关保留，默认关闭：

```bash
cmake .. -DMIRIX_ENABLE_CAPTURE=ON -DMIRIX_ENABLE_GUI=ON -DMIRIX_ENABLE_API=ON
```

> 桌面版的向量检索走 SQLite 暴力余弦；`EmbeddingIndexManager` 的 FAISS 方法仍是占位符。
> 桌面版定位与边界见 git 历史（`main` 分支）。

## 与 MIRIX 上游的关系

记忆分层架构（Core / Episodic / Semantic / Procedural / Resource / Knowledge Vault + Recall）参考自 [Mirix-AI/MIRIX](https://github.com/Mirix-AI/MIRIX)（Apache-2.0）。本仓库复用其记忆存储层设计，端侧推理与记忆编排为本仓库新写。

## 文档

- [架构文档](docs/architecture.md)
- [数据库设计](docs/database.md)
- [记忆系统](docs/memory.md)
- [测试说明](docs/testing.md)
