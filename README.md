# memolink — 端侧 Agent 记忆系统（Jetson Orin）

> 🎬 **在线演示**（均为 Orin 真实转录回放，非模拟、非指标陈列；两段演示已合并到同一页、顶部标签切换）：
> - 🧠 **记忆系统生命周期**：<https://ranpin.github.io/memolink/> —— 提取 → 消解 → 检索 → 矛盾更新 → 版本回溯 → 删除
> - 🚗 **座舱助手**（记忆系统驱动的车载应用）：<https://ranpin.github.io/memolink/#cockpit> —— SUPERSEDE 版本化 + 类型特化检索 + **多驾驶员治理拦截** + **摄像头多模态观察（Q9）**

在 **Jetson Orin** 上离线跑通的**多类型 Agent 记忆系统**：记忆的**提取 → 消解 → 存储 → 检索 → 更新 → 删除 → 版本回溯**全程在本地完成，不依赖云端。C++ 实现——自写 WordPiece 分词器 + ONNX INT8 推理做本地 embedding（与 HuggingFace 逐 token 对拍 25/25、与 Python onnxruntime 余弦 1.00000000），本地 4B LLM 做矛盾消解与事实提取、本地 Qwen3-VL-4B 做摄像头帧 caption（多模态记忆 Q9）。**v2 记忆系统本体与端侧推理均为本仓库新写**。

## 核心结果（Orin 实测，可复现）

| 闸门 | 内容 | 实测判据 |
|---|---|---|
| **Q1** | 版本化 CRUD（独立 `memory` 表 + 非破坏性版本化） | 41/41 单元测试 |
| **Q2** | 五操作消解（三层判定 + System-2 本地 4B） | 五分类 70%、可记四类 84%、目标定位 14/14 |
| **Q3** | 类型特化检索（意图路由 + 同类型加权 + 时间衰减） | 条件性提升：v2 基准 hit@1 **+31.2pp**（episodic 3/8→8/8） |
| **Q4** | 受约束提取（本地 4B，JSON 数组输出） | 编造率 **3.7%**（上界）、答案查全 56.7%、JSON 解析 98.4% |
| **Q5** | LongMemEval 接入 + 跨语言 config-only 切换 | s 变体 recall@10=0.78 / MRR=0.62；中英切换 5/5（dim 512↔384） |
| **Q6** | `memolink` CLI 八命令 + 生命周期 demo | 八命令 Orin 全跑通；线上 Playwright 23/23 |
| **Q7** | 治理标签过滤 + 强度衰减淘汰 | test_memory_governance 27/27（owner/scope/authority 过滤 + episodic 先淘汰 / procedural 最后） |
| **Q9** | 多模态记忆（帧→VLM caption→observed 记忆）+ authority 感知消解 | M2 authority 裁决 **18/18**（冲突场景集）；M3 既有闸门不回归；M4b 关键字段 **8/9**（14 帧 Commons 真实图）+ 钉分辨率 + 二次确认降级；详见 [Q9 方案](docs/q9_multimodal_plan.md) |

数字均由仓库内程序在 Orin 实跑生成、可复现，与 LLM 生成解耦。**生命周期五操作、三层消解、类型特化检索、治理与淘汰、authority 感知消解与多模态记忆等机制细节，以及诚实边界、跨域扩展**详见 [v2 设计文档](docs/memory_v2_design.md)。

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

## 应用示例：座舱助手

`cockpit_assistant` 是记忆系统驱动的**车载对话应用**，与 `memolink` CLI 共用同一个记忆核——同一套记忆系统既能驱动命令行工具，也能驱动车载助手。一段多轮对话点亮独有机制：

- **SUPERSEDE 版本化**：搬家后地址更新，旧值失效保留、`history` 可回溯
- **类型特化检索**：「上次充电在哪」→ episodic、「座椅加热怎么开」→ procedural
- **多驾驶员治理**：换驾驶员后，他人私人记忆（住址/孩子学校）被拦截，共享记忆（车辆参数）仍可见
- **摄像头多模态观察（Q9）**：座舱帧 → 本地 Qwen3-VL caption → 二次确认 → 以 observed 可信度入库；与口述（asserted）冲突时按可信度裁决（observed 取代 asserted、保留版本链），文字复读不一致则降为 inferred

```bash
make -j8 cockpit_assistant                                    # 构建（需 EDGE=ON + 本地 llama-server）
./cockpit_assistant run ../bench/cockpit_demo_script.json     # 脚本化对话（demo 转录来源）
./cockpit_assistant chat --owner 张三                          # 交互式
```

> **诚实边界**：消解与提取由本地 4B 完成、有已知误差（五分类 ~70%、提取查全 ~57%，详见[设计文档 §15](docs/memory_v2_design.md)），故 demo 脚本用叙述性、单值矛盾的话术让机制稳定触发；治理是检索期访问控制、不是 unlearning（被拦记忆仍在库、对 owner 可见）；回复由本地 qwen3-4b 生成、是跑通闭环的配角而非贡献点（贡献点是记忆系统本身）；**Q9 多模态是「视觉接地的文本记忆」（caption-then-embed），不是跨模态检索/以图搜图，VLM 同为配角、贡献点是 authority 感知消解；caption 误读会以 observed 最高可信度静默写库，二次确认只挡读数漂移、挡不住稳定误读**；单设备、未测热降频与功耗。

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

> 旧桌面端系统代码（采集/GUI/API/agents）已整体移入 `src/legacy/` 并**移出构建**（2026-10-22），留盘仅作参考、不参与端侧构建（该目录第三方代码署名见 `src/legacy/NOTICE`）；端侧构建只依赖 sqlite3 + curl + jsoncpp + onnxruntime。

## 文档

- **[v2 记忆系统设计](docs/memory_v2_design.md)** —— 权威方案 · Q1–Q7 + Q9 闸门 · 实测结果 · 关键机制 · 诚实边界
- **[Q9 多模态记忆方案](docs/q9_multimodal_plan.md)** —— caption-then-embed · authority 感知消解 · M1–M4 闸门 · caption 实测基线 · 诚实边界
- [架构文档](docs/architecture.md) · [数据库设计](docs/database.md) · [测试说明](docs/testing.md)
