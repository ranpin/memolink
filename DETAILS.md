# MIRIX-Style C++ 多模态记忆系统实现细节文档

## 1. 总体架构（严格 MIRIX 命名）

- **AgentWrapper**：主控代理生命周期管理
- **SessionManager**：会话窗口与 session 关联
- **RecallMemoryManager**：短期记忆（recall_memory）
- **ArchivalMemoryManager**：长期记忆（六大类型分表）
- **各类型 Agent**：CoreMemoryAgent, EpisodicMemoryAgent, SemanticMemoryAgent, ProceduralMemoryAgent, ResourceMemoryAgent, KnowledgeVaultAgent
- **ModelManager**：多模态模型推理（ONNX、pybind11、REST）
- **ScreenActivityTracker/CameraCaptureManager**：多模态采集
- **EmbeddingIndexManager**：faiss 向量索引
- **UserManager/BlockManager**：用户与 core memory block 管理
- **AuditLogManager**：操作日志
- **BackupManager**：数据备份与恢复
- **APIServer**：REST/gRPC SDK接口
- **UI/CLI**：本地界面

---

## 2. 数据库表结构（部分字段可详见 MIRIX 源码）

### 2.1 recall_memory（短期记忆）
| id | user_id | session_id | agent_id | memory_type | content | metadata | created_at |
- memory_type: text, image, file, audio, event等
- metadata: JSON，包含标签、来源、状态等

### 2.2 六大长期记忆类型
#### core_memory_block
| id | user_id | label | value | encrypted_value | last_updated |
- label: persona, human, user_preferences等

#### episodic_memory
| id | user_id | agent_id | timestamp | summary | details | event_type | tree_path | metadata |

#### procedural_memory
| id | user_id | agent_id | title | summary | steps | tree_path | metadata |

#### resource_memory
| id | user_id | agent_id | filename | resource_type | summary | content | last_accessed | size | tree_path | metadata |

#### semantic_memory
| id | user_id | agent_id | name | summary | details | tree_path | metadata |

#### knowledge_vault
| id | user_id | agent_id | caption | entry_type | source | sensitivity | encrypted_value | metadata |

### 2.3 audit_log（操作日志）
| id | user_id | agent_id | action_type | memory_id | memory_type | timestamp | metadata | session_id |

### 2.4 embedding_index
| id | memory_id | memory_type | embedding | metadata |

### 2.5 user, session, agent
| id | name | ... |
| id | user_id | start_time | end_time | status | ... |
| id | name | type | status | config | ... |

---

## 3. 主要模块实现细节

### 3.1 RecallMemoryManager

- 新增：`add_message`（每次采集/输入/上传自动调用）
- 查询：`get_recall_memory_summary`, `list_messages`, `filter_messages`
- 编辑：`edit_message`（内容、标签、类型、元数据）
- 删除：`delete_message`（单条/批量/条件清理）
- 归档：`archive_message`（转为长期记忆类型，调用 ArchivalMemoryManager）
- 激活：`activate_archival_memory`（把长期记忆拉入 recall 窗口）

### 3.2 ArchivalMemoryManager

- 新增：`add_*_memory`（如 add_episodic_memory, add_resource_memory 等）
- 查询：`list_*_memories`, `search_*_memories`（结构化/全文/embedding检索）
- 编辑：`update_*_memory`（内容、标签、元数据、embedding）
- 删除：`delete_*_memory`（单条/批量/条件清理）
- 激活：`activate_*_memory`（加入 recall_memory/会话窗口）

### 3.3 AuditLogManager

- 记录所有 memory 增删查改、归档、激活、检索操作
- 日志内容：操作类型、用户、agent、会话、目标记忆id/type、时间、元数据
- 提供查询接口 `/audit/logs`，支持多条件筛选

### 3.4 BackupManager

- 一键备份所有相关表为加密/压缩文件
- 恢复时解密还原所有数据表
- 支持本地和远程备份路径（load_from）
- 备份内容包括 memory、embedding、audit_log、用户表等

### 3.5 加密与隐私

- `encrypted_value` 字段用于 knowledge_vault、core_memory_block、credentials
- 加密算法：AES，密钥本地保存，用户认证方可解密
- 用户隔离：所有 memory/audit_log 均带 user_id，严格权限控制

### 3.6 多模态采集/推理

- ScreenActivityTracker/CameraCaptureManager 定时/事件驱动采集，自动生成 message
- ModelManager 推理后自动生成 embedding，归档到 embedding_index，关联 memory_id/type
- 所有采集内容自动写入 recall_memory，用户可批量归档为长期记忆

### 3.7 Agent体系与会话窗口

- 每类记忆有专属 Agent，负责采集/推理/归档/检索
- SessionManager 自动聚合当前会话相关 recall_memory
- AgentManager 管理 Agent 生命周期、协作、任务分派

### 3.8 API/SDK方法

- `send_message`, `add`, `edit_message`, `delete_message`, `archive_memory`, `activate_archival_memory`
- `add_core_memory_block`, `list_core_memory_blocks`, `update_core_memory_block`, `delete_core_memory_block`
- `list_episodic_memories`, `add_episodic_memory`, `update_episodic_memory`, `delete_episodic_memory`
- `visualize_memories`, `get_recall_memory_summary`, `get_archival_memory_summary`
- `embedding_search`, `bm25_search`, `keyword_search`
- `audit/logs`
- `backup/create`, `backup/restore`
- `user/list`, `session/list`, `agent/list`

### 3.9 查询实现

- 结构化 SQL 查询（按字段、时间、类型、标签、用户、会话等）
- 全文检索（BM25/LIKE）
- embedding 向量检索（faiss，支持跨类型召回）
- 聚合统计/分类展示（visualize_memories）
- 权限/隔离控制（只查当前用户/session/agent关联）

---

## 4. 增删查改操作时机和业务流

- 新增（Create）：每次用户输入/上传/采集、Agent自动采集均自动写入 recall_memory，归档时写入长期记忆
- 查询（Read）：用户主动、Agent自动、模型上下文拉取、会话窗口聚合，支持多种筛选与检索
- 编辑（Update）：用户或Agent主动修改内容、标签、元数据、embedding，支持合并/去重
- 删除（Delete）：用户主动、系统过期自动、批量清理
- 归档（Archive）：用户主动、系统定时、会话结束自动归档 recall_memory 至长期记忆
- 激活（Activate）：用户/Agent将长期记忆激活到 recall_memory，扩展会话窗口上下文
- 所有操作自动写入 audit_log

---

## 5. 日志与审计

- 记录所有 CRUD、归档、激活、检索操作，带 user_id/agent_id/session_id/memory_type/memory_id
- 用户可通过 API/界面随时查询、回溯操作历史
- 日志支持筛选、导出、批量分析

---

## 6. 数据备份/恢复

- BackupManager 提供 API/CLI 命令一键备份所有 memory 相关表
- 支持加密/压缩备份文件，安全存储本地或远程
- 恢复流程自动解密还原所有表，恢复用户/会话/记忆/embedding/日志
- 用户可选择定期自动备份、手动备份、指定恢复点

---

## 7. 安全与隔离

- 敏感信息加密存储，用户认证解密
- 严格用户/会话/Agent权限隔离
- 支持本地密钥管理、加密字段批量解密/导出

---

## 8. 扩展建议

- 插件式 Agent/模型/采集扩展接口
- 数据导出/迁移（支持多设备同步）
- 性能优化（异步采集、批量归档、向量并行检索）
- UI/CLI/桌面通知/本地 Web UI 可选开发

---

## 9. 总结

- 实现方案全部命名、流程、API、表结构与 MIRIX 保持一致。
- 支持 recall/archival 分层、六大记忆类型、embedding 检索、日志审计、加密、备份、用户隔离。
- 每个增删查改操作和业务流均有明确 API、时机和日志追踪。
- 如需某模块的具体 C++ 类设计、API参数、表结构代码、日志/加密/备份实现伪代码，请进一步指定。
