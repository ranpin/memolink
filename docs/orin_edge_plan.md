# memolink → Orin 端侧记忆系统演进计划

- **分支**：`orin-edge`（`main` 保留桌面版，不删任何桌面代码）
- **定案**：2026-10-20，用户已批准全部架构决策 + `apt` 装包 + 分支策略
- **状态**：P0 待开工
- **基线**：`7647be3`（main 的卫生+诚实性修复之后）

---

## 1. 项目定义与范围

### 1.1 是什么

端侧**多类型 agent 记忆系统**：记忆的**写入 / 组织 / 检索 / 淘汰**全部在 Jetson Orin 本地完成，
检索质量有**标注评测**，且记忆抽象**可跨域扩展**（座舱 / 机器人 / 自动驾驶）而**不改一行代码**。

### 1.2 不是什么（明确砍掉，后续不要再提议）

| 砍掉项 | 原因 |
|---|---|
| 并发 / 调度 / 内存预算强制 / 持续负载 / 热降频 | 属 agent harness / runtime，非本项目功能焦点（用户 2026-10-20 明确否掉） |
| 生成的 roofline 分析 | 生成不是本项目贡献点 |
| 自做 INT8 量化 | 属编码器工程，不属记忆系统 |
| LLM 生成质量优化 | 生成只是跑通闭环的**配角** |
| 端云融合的云端实测 | 无 API key；云端路径只实现接口、不出对比数据 |

### 1.3 为什么是 C++、为什么在 memolink 上演进

**诚实前提：C++ 重写买不到吞吐。** Python 原型实测端到端 929ms，其中生成占 920ms（99%）；
重写外围只影响那 9ms。C++ 的真实收益是：

1. **记忆系统本体就是那 2,618 行 C++**（memolink 的 6+1 类 manager 是真实现）——复用它比在 Python 里重写一遍更合理；
2. **依赖面最小化**：无 Python 运行时、无 Rust tokenizer、无 FAISS，只剩 onnxruntime + sqlite3；
3. **冷启动**：Python 要 import numpy/onnxruntime/tokenizers/faiss（实测进程 RSS 126MB），C++ 加载 ONNX session 仅 0.03–0.12s。⚠️ **此项尚未实测，不得先声称**；
4. 对上求职定位（C++ / 端侧系统）。

---

## 2. 系统架构

### 2.1 分层与 API 边界

```
┌─────────────────────────────────────────────────────────────────┐
│  消费方（配角，可替换）                                            │
│    · LLM 生成 demo（llama-server，本地）                          │
│    · 评测 harness（bench/，只打记忆系统，不碰生成）                 │
└───────────────────────────┬─────────────────────────────────────┘
                            │  只通过下面三个接口
┌───────────────────────────▼─────────────────────────────────────┐
│  记忆系统 API（本项目交付物）                                      │
│      write(memory)              写入 + 编码 + 落库 + 触发淘汰       │
│      retrieve(query) → 排序记忆  编码 + 跨类型检索 + 合并           │
│      evict()                    有界淘汰                          │
├─────────────────────────────────────────────────────────────────┤
│  MemoryOrchestrator  ← 当前是空壳，P3 实现（本项目核心贡献）        │
├──────────────┬──────────────────────┬───────────────────────────┤
│ 编码服务      │ 记忆存储              │ 配置                       │
│ WordPiece    │ 6+1 类 manager        │ ConfigManager             │
│ 分词器(新写)  │ (2,618 行，复用)       │ (231 行，复用)             │
│ ONNX C API   │ StorageManager        │ config.ini                │
│ INT8(新写)    │ + InitDB schema(复用)  │ + 域/语料路径(新增)         │
│              │ + embedding BLOB 持久  │                           │
└──────────────┴──────────────────────┴───────────────────────────┘
                            │
                    SQLite（每域一个库文件）
```

### 2.2 架构决策（2026-10-20 全部定案）

**A. 系统边界**：记忆系统暴露 `write / retrieve / evict` 三个接口，**LLM 生成与评测都是消费方**。
这样它才是「系统」而非 pipeline，评测也只打记忆系统本身（见 E）。

**B. 记忆生命周期**（目前只有「检索」，其余三项要补）：

- **B1 embedding 生命周期**：**写入时算一次 → 存进 `embedding BLOB` → 检索走内存缓存**。
  不是每次查询逐行读库（现有 `searchByEmbedding` 就是每条 `getEmbedding(id)` 读一次 SQLite）。
  启动时把 embedding 载入内存矩阵，写入/淘汰时增量更新——**SQLite 管持久、内存管检索**。
  每条 embedding 记一个**模型版本 tag**（如 `bge-small-zh-int8`），换模型时能检出失配。
- **B2 写入路径不能省**：只灌静态语料 = RAG 不是记忆系统。做成
  **种子语料（带类型、预加载）+ 实时写入（每轮对话落 recall/episodic，`ChatAgent` 已有此模式）+ 淘汰**。
  ⚠️ 不做「LLM 智能判断该存什么类型」——那是另一个大坑，v1 不碰。
- **B3 淘汰粒度**：**分类型容量上限 + 类型内 FIFO**（不是全局一条 FIFO）。
  淘汰一条 Procedural（学到的技能）与淘汰一条 Episodic（闲聊）不该同等对待。
  设计承自 aadkMemory 的 `id_queue` + `remove_ids` FIFO。

**C. 跨类型检索合并策略**：v1 = **全类型都搜 + 按归一化分数合并取 top-k**（简单、可解释）。
路由式检索（先判 query 该查哪类）列为后续可说点，v1 不做。

**D. 多域布局**：**每个域 = 一份语料文件 → 一个独立 SQLite 库**，`config.ini` 指定加载哪个。
「换域」= 改配置 + 重建库，代码零改动；也避免三域记忆混库互相污染。

**E. 评测边界**：评测对象是**记忆系统的检索质量，与生成解耦**。
好处：① 不需要 LLM 也能评记忆系统；② 4B 模型会编造（已实测）**不会污染**记忆系统结论。
标注 GT = 「query → 应命中的 memory id 列表」，独立成语料之外的评测集。

### 2.3 跨域可扩展性（用户 2026-10-20 要求，属记忆系统本职）

memolink 的 6+1 类记忆源自认知科学分类，**本就领域无关**：

| 类型 | 座舱 | 机器人 | 自动驾驶 |
|---|---|---|---|
| Semantic | 用户喜欢深蓝色车 | 红方块在桌子左侧 | 该路段限速 60 |
| Episodic | 上次保养 8 月 / 1.2 万 km | 上次抓取失败因夹爪偏 15° | 上次在该路口急刹 |
| Procedural | 说「我有点冷」→ 调高 2 度 + 座椅加热 | 抓取：移到上方→下降→闭合→抬起 | 雨天提前 100m 减速 |
| Core | 当前空调 24 度 | 当前任务：方块放进盒子 | 驾驶员不喜欢急加速 |
| Resource | 截屏 / 音频引用 | 相机帧 / 点云引用 | 摄像头帧 / 地图引用 |

**同一套类型、同一个二进制、同一套检索，换域只换语料。**
⚠️ 诚实边界：三域语料均为手写合成、标注自给，证明的是「记忆抽象换域不需改代码」，
**不是**「在真机器人 / 真车上部署过」——措辞不得过线。

---

## 3. 复用 / 新写 / 保留（逐文件核实过）

### ✅ 复用（已核实为真实现）

| 组件 | 行数 | 核实依据 |
|---|---|---|
| 6+1 类记忆 manager | **2,618** | `RecallMemoryManager::searchByEmbedding` 逐行读过：取全量 → `getEmbedding` → 内联余弦 → 排序 topK。是真实现，「简化实现」注释指**没用索引**而非没实现 |
| `InitDB` schema | — | 12+ 张表；`recall_memory` **已有 `embedding BLOB` 列** |
| `StorageManager` | 122 | 仅依赖 `<sqlite3.h>` |
| `ConfigManager` | 231 | 仅依赖 `<string>` `<map>` |
| `VectorSerializer` | 112 | 裸 float 字节序列化，配 BLOB 正好 |
| `ChatAgent::buildContext` | 71 | 拉最近 N 条 recall memory 拼上下文 + 落库，写入路径可抄 |

### 🆕 新写（端侧必需，两个旧仓都没有）

| 组件 | 预估 | 说明 |
|---|---|---|
| C++ WordPiece 分词器 | ~200 行 | `tokenizer.json` 已核实 = WordPiece + **21128 词表** + BertNormalizer + BertPreTokenizer。砍掉 Rust `tokenizers` 依赖 |
| ONNX Runtime **C API** 编码 | ~150 行 | 替换 `TextEmbeddingGenerator` 的**实现**、**保持接口不变** ⇒ 2,618 行 manager 零改动 |
| `MemoryOrchestrator` 实现 | ~200 行 | 填四个空壳 + B1/B2/B3/C 的策略 |
| 路由（隐私/复杂度） | ~100 行 | 隐私硬约束 + 复杂度启发式 |
| 评测 harness | ~150 行 | 标注 GT → recall@k / MRR，三域各跑一遍 |

### 🔶 保留但改造（**桌面版不删**）

- `capture/`(1605 行，含 Mac `.mm`)、`gui/`(Qt)、`api/`(HTTP server) → **CMake option 关掉，代码保留**
- `CLIPProcessor` 里写死的 `/Users/ranpin/code/MIRIX/mirix_env/bin/python` → **移到配置项**
- `QwenAPI`（cURL）→ 指向本地 llama-server，**保留它原有的 local/cloud 切换**

```cmake
option(MIRIX_ENABLE_CAPTURE "相机/音频/截屏采集（桌面）"   OFF)
option(MIRIX_ENABLE_GUI     "Qt GUI（桌面）"              OFF)
option(MIRIX_ENABLE_API     "HTTP API server（桌面）"      OFF)
option(MIRIX_ENABLE_EDGE    "Orin 端侧推理（ONNX+llama）"  ON)
```

### ❌ 不需要

- **FAISS**：F2 已证明精确检索够（见 §6），且 memolink 的 `VectorSearchManager` 本来就是暴力余弦。
  连带 **Eigen3 也不需要**（dog 上没有）。`EmbeddingIndexManager` 的 4 个 FAISS 占位符方法保持占位、不接。

---

## 4. 闸门 P0–P5

| 闸门 | 内容 | 判据（可验证） |
|---|---|---|
| **P0** | 依赖 + 构建基线：`apt install libsqlite3-dev`；下 onnxruntime aarch64 C release（含头）；memolink 同步到 dog | `cmake` configure 通过 |
| **P1** | CMake 开关化（桌面代码保留、默认不编）；Orin 编译过 | `MIRIX_ENABLE_EDGE=ON` 在 Orin 编译通过，**且桌面配置在 Mac 上仍能编**（没破坏桌面版） |
| **P2** | C++ WordPiece 分词器 + ONNX C API 跑 INT8，替换 `TextEmbeddingGenerator` 实现（接口不变） | 与 Python `tokenizers` **逐 token 对拍一致**；embedding 与 Python 参考**余弦 ~0.99** |
| **P3** | 实现 `MemoryOrchestrator`：按类型写入 + 跨类型检索合并（C）+ 实时写入（B2）+ 分类型有界淘汰（B3）+ embedding 持久化与内存缓存（B1）+ 语料配置驱动（D） | 写入→检索→淘汰闭环跑通；重启后 embedding 从库恢复、不重算 |
| **P4** | **三域检索评测（核心交付）**：座舱 40 + 机器人 ~25 + AV ~25，全部带标注 GT，**同一二进制跑三遍**；并测 **INT8 vs FP32 的 top-k 命中集合是否一致** | 三域各出 recall@k / MRR；量化对排序的影响有数 |
| **P5** | 生成保持最简（本地 llama-server）跑通闭环；README 定位演进；demo 页 + demo 视频 | 端到端演示可跑；README 顶部有在线链接 |

**P2 是最容易翻车的闸门**：C++ WordPiece 必须和 HF tokenizer 逐 token 对齐，否则 embedding 全错。
对拍方法：同一批输入，C++ 输出的 `input_ids` 与 Python `tokenizers` 输出**逐个相等**才算过。

---

## 5. 已核实的设备与依赖事实（dog = 30.245.40.99）

| 项 | 状态 |
|---|---|
| 硬件 | Jetson Orin，JetPack 6.2（R36.4.4），CUDA 12.6，12 核 ARM，**24G 可用**，/home 527G 可用 |
| ⚠️ 内存占用 | `dog-gpu-memory-occupied` 记忆里「26.5G 被 system_agent 占满」**已过期**；2026-10-20 实测仅 4.8G used。`system_agent`(pid 2485) 进程仍在但不占内存。**排大内存任务前仍先 `free -m` 复查** |
| 编译器 | g++ 11.4.0、cmake 3.22.1 ✅ |
| ❌ 缺的开发头 | **sqlite3 / curl / jsoncpp / openssl 全缺**；`apt` 可用（P0 装 `libsqlite3-dev`） |
| onnxruntime | Python 1.23.2，**CPU-only**（无 CUDA EP，GPU device discovery 失败）；**C 头文件缺**，需下 aarch64 release |
| llama.cpp | `~/chenrunbin/llama.cpp/build` **已 CUDA 构建**（`GGML_CUDA=ON`）；⚠️ `build/bin/` **只有 `llama-server`**，无 `llama-cli`/`llama-bench`；`libllama.so` + `include/llama.h` 在 |
| 本地模型 | `~/chenrunbin/qwen-trajectory-prediction/qwen3-4b-q4_k_m.gguf`（2.33G）。其余 38 个 `*.gguf` 是 `ggml-vocab-*` 测试夹具，**不是真模型** |
| 已装 Python 包 | faiss-cpu 1.15.1（有官方 aarch64 wheel）、tokenizers 0.22.2、numpy 1.26.4 |
| 端口 | 已占用 22 / 53 / 111 / **8086（car_control）** / 9876；本项目用 **8099** |
| embedding 模型 | `onnx-community/bge-small-zh-v1.5-ONNX`（hf-mirror 可下），512 维，**FP32 94.8MB / INT8 23.9MB**，⚠️ 用 ONNX external data，`.onnx` 与 `_data` 必须同目录同名 |

起 llama-server：
```bash
~/chenrunbin/llama.cpp/build/bin/llama-server \
  -m ~/chenrunbin/qwen-trajectory-prediction/qwen3-4b-q4_k_m.gguf \
  --host 127.0.0.1 --port 8099 -ngl 99 -c 4096
```
约 25s 就绪（日志出现 `server is listening` + `all slots are idle`）。

---

## 6. Python 原型已有实测结论（`bench/results/`，与语言无关，继续有效）

**F1 端侧 embedding（bge-small-zh-v1.5，onnxruntime CPU）**

| | th=1 | th=4 | th=8 | 模型 | RSS 增量 |
|---|---|---|---|---|---|
| FP32 | 33.13ms | 10.19ms | 5.92ms | 94.8MB | 76–82MB |
| **INT8** | **10.79ms** | **4.11ms** | **3.02ms** | **23.9MB** | **42–46MB** |

INT8 vs FP32 embedding 余弦：query 0.99156 / memory 0.99336（min 0.99048）。
两个非显然发现：① **INT8 单线程比 FP32 双线程还快** ⇒ 端侧可少占核；
② **INT8 线程扩展性更差**（th=1→8 仅 3.57×，FP32 是 5.6×）⇒ 受带宽/量化 kernel 并行度限制。

**F2 检索层（512 维，带簇结构合成向量，recall@5）**

| N | numpy 暴力 | faiss flat（精确） | HNSW ef=64 | HNSW 建索引 | HNSW 索引 |
|---|---|---|---|---|---|
| 1k | 0.062ms | **0.127ms** / 1.0 | 0.074ms / 0.998 | 0.25s | 2.3MB |
| 10k | 0.779ms | **0.787ms** / 1.0 | 0.171ms / 1.0 | 3.86s | 23.2MB |
| 100k | 9.80ms | **6.63ms** / 1.0 | 0.139ms / 1.0 | **36.1s** | 232MB |

⇒ **精确检索在 agent 记忆量级（1e3~1e4、增量更新）足够**，ANN 的建索引代价不可接受。
这是「不接 FAISS」的依据。

**F3 纯端侧闭环（离线、零 API key）**：5/5 查询成功，端到端均值 **929ms**
（embed 9.4 + 检索 0.16 + 生成 920）；隐私硬约束正确触发（「我住在哪里？」命中 2 条 sensitive ⇒ 强制端侧）。

**原型期抓出的 4 个测量假值**（都已修，教训写进了脚本注释）：
① F1 模型大小漏算 ONNX external data（报 0.0MB）；② F2 用 `ru_maxrss` 峰值导致索引内存测成 0；
③ F2 用纯随机向量把 ANN recall 压到 0.45（随机高维是 ANN 最坏情形，不代表真实 embedding）；
④ F3 `retrieve()` 内部重复编码，把 embed 耗时记到检索头上（8.1ms → 真实 0.16ms）。

---

## 7. 诚实边界（写进 README，不藏脚注）

1. **云端路径已实现但未实测**（无 key）⇒ 严格说证明的是「**纯端侧可行**」，不是「端云融合更优」。
2. **端侧 4B 模型会编造**：实测「你觉得我最近生活状态怎么样？」答出记忆里不存在的
   「早高峰时间略有增加（约+1分钟）」，尽管 prompt 明确禁止。这是小模型在开放式综合问题上的真实边界。
3. **onnxruntime 在 Orin 是 CPU-only** ⇒ 没有 GPU embedding 数据（CPU 是刻意选择：把 GPU 留给 LLM）。
4. **三域语料均为手写合成、标注自给** ⇒ 证明「换域不改代码」，不证明「真机部署过」。
5. **隐私路由依赖手工 `sensitive` 标注**，没有 PII 自动检测 ⇒ 措辞应为「基于敏感级标注的路由隔离」。
6. **单设备、未测热降频与功耗** ⇒ 数字是冷启动/短负载下的。

---

## 8. 风险

| 风险 | 等级 | 缓解 |
|---|---|---|
| C++ WordPiece 与 HF tokenizer 不一致 ⇒ embedding 全错 | **高** | P2 闸门强制逐 token 对拍，不过不进 P3 |
| onnxruntime C release 版本与 dog 上 `.so` 不匹配 | 中 | 下完整 release 包（含 include+lib），不自拼 |
| 剥离桌面代码时牵连 manager 依赖 | 中 | P1 用 CMake option 而非删代码，逐个关 |
| Orin 原生编译慢 | 低 | 12 核，可接受；不交叉编译（避工具链坑） |
| 三域语料标注主观 | 中 | 标注规则写进评测集 README，标注与检索实现分离 |
