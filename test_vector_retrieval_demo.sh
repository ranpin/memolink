#!/bin/bash

echo "=========================================="
echo "向量检索完整链路验证 Demo"
echo "=========================================="

cd /Users/ranpin/code/mirix-cpp/build

# 清理旧数据库
rm -f test_vector_demo.db
echo "✓ 清理旧数据库"

# 创建测试数据库并插入数据
sqlite3 test_vector_demo.db <<EOF
-- 创建用户表
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    created_at TEXT NOT NULL
);

-- 创建会话表
CREATE TABLE IF NOT EXISTS sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER NOT NULL,
    created_at TEXT NOT NULL
);

-- 创建消息记忆表
CREATE TABLE IF NOT EXISTS message_memories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    agent_id INTEGER NOT NULL,
    session_id INTEGER NOT NULL,
    content TEXT NOT NULL,
    created_at TEXT NOT NULL,
    processed INTEGER DEFAULT 0
);

-- 插入测试数据
INSERT INTO users (id, name, created_at) VALUES (1, 'TestUser', datetime('now'));
INSERT INTO sessions (id, user_id, agent_id, created_at) VALUES (1, 1, 1, datetime('now'));

-- 插入5条测试消息（触发记忆生成的阈值）
INSERT INTO message_memories (user_id, agent_id, session_id, content, created_at, processed) VALUES 
(1, 1, 1, '今天天气真好，适合出去散步', datetime('now', '-5 minutes'), 0),
(1, 1, 1, '我喜欢在公园里跑步锻炼身体', datetime('now', '-4 minutes'), 0),
(1, 1, 1, '公园里有很多漂亮的花', datetime('now', '-3 minutes'), 0),
(1, 1, 1, '我在公园遇到了一只可爱的小狗', datetime('now', '-2 minutes'), 0),
(1, 1, 1, '小狗的主人说它叫旺财', datetime('now', '-1 minutes'), 0);

SELECT '✓ 插入了 ' || COUNT(*) || ' 条测试消息' FROM message_memories;
EOF

echo ""
echo "=========================================="
echo "Phase 1: 运行记忆生成 (触发 embedding 向量化)"
echo "=========================================="

# 创建临时配置文件
cat > test_config.ini <<EOF
[database]
path = test_vector_demo.db

[memory_generation]
message_threshold = 3
time_interval_minutes = 30
confidence_threshold = 0.5
max_sources = 50
enable_provenance = true
enable_episodic = true
enable_semantic = true
enable_core = true
enable_procedural = true
enable_secret_vault = true
message_quota = 10
screenshot_quota = 5
audio_quota = 3
image_quota = 5
only_current_session = false

[api]
provider = qwen
api_url = https://dashscope.aliyuncs.com/compatible-mode/v1
api_key = sk-test-key
model = qwen-plus
temperature = 0.7
max_tokens = 2000
EOF

echo "✓ 配置文件已创建"

# 注意：这里需要实际的 API key 才能生成 embedding
# 如果没有 API key，embedding 生成会失败，但能看到调用流程
echo ""
echo "运行 test_memory_generation 查看记忆生成流程..."
echo "(注意：需要有效的 API key 才能真正生成 embedding)"
echo ""

./test_memory_generation 2>&1 | grep -E "(MemoryGenerationManager|Embedding|embedding|索引)" | head -50

echo ""
echo "=========================================="
echo "检查数据库中的表"
echo "=========================================="

sqlite3 test_vector_demo.db <<EOF
.tables
EOF

echo ""
echo "=========================================="
echo "完成！"
echo "=========================================="
echo ""
echo "说明："
echo "1. Phase 1 已完成：记忆生成时会调用 embedding 生成和索引"
echo "2. Phase 2 已完成：ChatAgent 使用 VectorSearchManager 检索"
echo "3. 要完整测试需要配置有效的 Qwen API key"
echo ""
echo "主要改动文件："
echo "  - src/memory/generation/MemoryGenerationManager.cpp (添加 embedding 生成)"
echo "  - src/models/EmbeddingIndexManager.cpp (实现向量索引)"
echo "  - src/models/VectorSearchManager.cpp (获取真实记忆内容)"
echo "  - src/memory/context/ContextBuilder.cpp (集成向量检索)"
echo "  - src/agents/ChatAgent.cpp (使用 VectorSearchManager)"
echo ""
