# memolink — 端侧 Agent 记忆系统（Jetson Orin）

> 🎬 **在线演示**：<https://ranpin.github.io/memolink/> —— 真实 CLI 转录的**生命周期分步回放**（提取 → 消解 → 检索 → 矛盾更新 → 版本回溯 → 删除），非模拟、非指标陈列。

在 **Jetson Orin** 上离线跑通的**多类型 Agent 记忆系统**：记忆的**提取 → 消解 → 存储 → 检索 → 更新 → 删除 → 版本回溯**全程在本地完成，不依赖云端。C++ 实现——自写 WordPiece 分词器 + ONNX INT8 推理做本地 embedding（与 HuggingFace 逐 token 对拍 25/25、与 Python onnxruntime 余弦 1.00000000），本地 4B LLM 做矛盾消解与事实提取。记忆分层架构参考自 [Mirix-AI/MIRIX](https://github.com/Mirix-AI/MIRIX)（Apache-2.0），**v2 记忆系统本体与端侧推理为本仓库新写**。

## 核心结果（Orin 实测，可复现）

| 闸门 | 内容 | 实测判据 |
|---|---|---|
| **Q1** | 版本化 CRUD（独立 `memory` 表 + 非破坏性版本化） | 41/41 单元测试 |
| **Q2** | 五操作消解（三层判定 + System-2 本地 4B） | 五分类 70%、可记四类 84%、目标定位 14/14 |
| **Q3** | 类型特化检索（意图路由 + 同类型加权 + 时间衰减） | 条件性提升：v2 基准 hit@1 **+31.2pp**（episodic 3/8→8/8） |
| **Q4** | 受约束提取（本地 4B，JSON 数组输出） | 编造率 **3.7%**（上界）、答案查全 56.7%、JSON 解析 98.4% |
| **Q5** | LongMemEval 接入 + 跨语言 config-only 切换 | s 变体 recall@10=0.78 / MRR=0.62；中英切换 5/5（dim 512↔384） |
| **Q6** | `memolink` CLI 八命令 + 生命周期 demo | 八命令 Orin 全跑通；线上 Playwright 23/23 |
| **Q7** | 治理标签过滤 + 强度衰减淘汰 | test_memory_governance 21/21（owner/scope/authority 过滤 + episodic 先淘汰 / procedural 最后） |

数字均由仓库内程序在 Orin 实跑生成、可复现，与 LLM 生成解耦。**生命周期五操作、三层消解、类型特化检索、治理与淘汰等机制细节，以及诚实边界、跨域扩展**详见 [v2 设计文档](docs/memory_v2_design.md)。

## CLI：`memolink` 八命令

```bash
memolink extract  --user "..." --assistant "..."   # ① 提取候选事实
memolink ingest   "..." --type semantic            # ②③ 消解 + 存储
memolink recall   "..." --top 4 [--type T]         # ④ 类型特化检索
memolink update   <id> "..."                       # 显式更新
memolink remove   <id>                             # 记录级删除
memolink list     [--type T] [--invalid]           # 列举
memolink history  <id>                             # 版本链
memolink stats                                     # 统计
```

## 构建与复现（Orin）

```bash
sudo apt install -y libsqlite3-dev libcurl4-openssl-dev libjsoncpp-dev
# config.ini 的 [edge] 段填本地 embedding 模型与分词器路径
# config.ini 的 local_url 填本地 llama-server 地址（System-2 消解 + 提取用）
mkdir build && cd build
cmake .. -DMEMOLINK_ENABLE_EDGE=ON
make -j8 memolink
./memolink ingest "用户喜欢深蓝色的车" --type semantic
./memolink recall "我的车是什么颜色偏好" --top 3
```

> 旧 MIRIX 桌面系统（采集/GUI/API/agents）已整体移入 `src/legacy/` 并**移出构建**（2026-10-22），代码留盘仅作参考；端侧构建只依赖 sqlite3 + curl + jsoncpp + onnxruntime。

## 文档

- **[v2 记忆系统设计](docs/memory_v2_design.md)** —— 权威方案 · Q1–Q7 闸门 · 实测结果 · 关键机制 · 诚实边界
- [架构文档](docs/architecture.md) · [数据库设计](docs/database.md) · [测试说明](docs/testing.md)
