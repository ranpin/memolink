# 测试说明（v2 端侧记忆系统）

> 本文描述 **memolink v2 记忆系统**的测试与评测。全部数字由仓库内程序在 **Jetson Orin** 实跑生成、可复现，与 LLM 生成解耦。权威方案与判据见 [memory_v2_design.md](memory_v2_design.md) §12 / §14。旧 MIRIX v1 测试文档已随 legacy 归档至 `src/legacy/docs/`。

## 测试分层

- **闸门测试（Q1–Q7）**：v2 记忆系统各机制的验收测试，全部为 C++ 可执行程序（Q4 为 Python 评测脚本），在 Orin 上实跑。
- **基础设施测试**：embedding / 分词器 / 向量序列化的对拍与接线验证。
- **生命周期 demo**：`memolink` CLI 八命令 + 线上生命周期回放页（Q6）。

## Q1–Q7 闸门测试矩阵

| 闸门 | 测试载体 | 内容 | 实测判据 |
|---|---|---|---|
| **Q1** | `test_edge_memory_store` | 版本化 CRUD（独立 `memory` 表 + 非破坏性版本化） | 41/41 单元测试 |
| **Q2** | `test_memory_resolver` | 五操作消解（三层判定 + System-2 本地 4B） | 五分类 70%（21/30）、可记四类 84%（21/25）、目标定位 14/14、System-1 覆盖 43% |
| **Q3** | `test_typed_retrieval` | 类型特化检索（意图路由 + 同类型加权 + 时间衰减） | 条件性提升：v2 基准 hit@1 **+31.2pp**（episodic 3/8→8/8）；v1 基准触顶 +0；意图识别 100% |
| **Q4** | `bench/extract_eval.py` | 受约束提取（本地 4B，JSON 数组输出） | 编造率 **3.7%**（上界）、答案查全 56.7%、JSON 解析 98.4% |
| **Q5** | `test_longmemeval_retrieval` | LongMemEval 接入 + 跨语言 config-only 切换 | s 变体 recall@10=0.7822 / MRR=0.6203；中英切换 5/5（dim 512↔384） |
| **Q6** | `memolink` CLI + demo 页 | 八命令串完整管线 + 生命周期回放 | 八命令 Orin 全跑通；线上 Playwright 23/23 无控制台错误 |
| **Q7** | `test_memory_governance` | 治理标签过滤 + 强度衰减淘汰 | 27/27（owner/scope/authority 过滤 + episodic 先淘汰 / procedural 最后 + reinforce 耐淘汰） |

> Q4 是 Python 评测脚本（`bench/extract_eval.py`），用 LongMemEval oracle 确定性分层子集（30 题/每题型前 5）评测本地 4B 的受约束提取；其余闸门为 C++ 测试可执行文件。

## 基础设施测试

| 测试 | 内容 | 实测判据 |
|---|---|---|
| `test_tokenizer_parity` | 自写 C++ WordPiece 分词器 vs HuggingFace `tokenizers` | 25/25 逐 token 一致（zh/en/混排/标点/emoji/ja/ko） |
| `test_embed_parity` | C++ ONNX Runtime vs Python `onnxruntime` embedding | FP32 余弦 1.00000000（max diff 2.9e-7） |
| `test_edge_embedding` | TextEmbeddingGenerator 走本地 ONNX 接线 | 5/5（维度正确、norm=1、全程不打云端） |
| `test_vector_serializer` | 向量序列化 / 反序列化 | 往返一致 |

## 运行方式（Orin）

```bash
mkdir build && cd build
cmake .. -DMEMOLINK_ENABLE_EDGE=ON
make -j8

# 单个闸门测试
./test_edge_memory_store      # Q1
./test_memory_resolver        # Q2（需本地 llama-server，见 config.ini local_url）
./test_typed_retrieval        # Q3
./test_longmemeval_retrieval  # Q5（需 bench/data/ 的 LongMemEval 数据）
./test_memory_governance      # Q7

# 基础设施对拍
./test_tokenizer_parity
./test_embed_parity
./test_edge_embedding
./test_vector_serializer

# Q6：CLI 八命令
./memolink stats
```

- Q2 / Q4 / Q6 依赖本地 LLM：`config.ini` 的 `local_url` 填本地 llama-server 地址（如 `http://127.0.0.1:8099`）。
- embedding 依赖 `config.ini` 的 `[edge]` 段（模型 + 分词器路径）；留空则回退云端/模拟。
- Q5 依赖 `bench/data/` 的 LongMemEval 数据（已 gitignore，仅在 dog 上）。

## 关于 v1 遗留测试

旧 v1 / P3-P4 时代依赖 legacy 代码的测试（`test_memory_orchestrator` / `test_retrieval_eval` / `test_system_initialization` / `test_embedding_comprehensive` / `test_chat_with_model` 及 CAPTURE 系列）已于 2026-10-22 随 legacy **移出构建**，源文件留盘于 `test/` 仅作参考，不再编译运行。它们对应的功能已被 Q1–Q7 闸门测试取代。
