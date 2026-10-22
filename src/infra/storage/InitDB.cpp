
#include <vector>
#include <string>
#include "infra/storage/InitDB.h"
#include "infra/storage/StorageManager.h"
#include <iostream>

bool InitDB::initializeDatabase(const std::string& dbPath) {
    StorageManager storage(dbPath);
    if (!storage.initialize()) {
        std::cerr << "Failed to initialize database connection" << std::endl;
        return false;
    }

    // ---- v2 端侧记忆表（docs/memory_v2_design.md §10）----
    // 刻意**不加** user/agent/session 外键：recall_memory 那三个外键配合
    // StorageManager::initialize() 里的 PRAGMA foreign_keys=ON，曾让 P3 首跑
    // 40/40 条写入全部违约失败（修法见 MemoryOrchestrator::ensureDefaultEntities）。
    // v2 表自成一体，不依赖那三张表，从根上避开这个坑。
    //
    // 注：v1 的 12 张表（user/agent/session/core_memory_block/episodic_memory/
    // semantic_memory/procedural_memory/resource_memory/knowledge_vault/
    // archival_memory/recall_memory/audit_log）已随 legacy 代码移出构建
    // （2026-10-22），不再由本函数创建；见 docs/architecture.md「v1 设计遗产」。
    std::string createMemoryTable = R"(
        CREATE TABLE IF NOT EXISTS memory (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            type            TEXT NOT NULL,
            content         TEXT NOT NULL,
            embedding       BLOB,
            embed_model     TEXT,
            strength        REAL DEFAULT 1.0,
            access_count    INTEGER DEFAULT 0,
            owner           TEXT DEFAULT 'user',
            scope           TEXT DEFAULT 'private',
            authority       TEXT DEFAULT 'asserted',
            valid_from      TEXT,
            valid_until     TEXT,
            superseded_by   INTEGER,
            supersedes      INTEGER,
            source_session  TEXT,
            source_turn     INTEGER DEFAULT 0,
            created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    if (!storage.executeNonQuery(createMemoryTable)) {
        std::cerr << "Failed to create memory table" << std::endl;
        return false;
    }
    for (const char* idx : {
             "CREATE INDEX IF NOT EXISTS idx_memory_type_valid ON memory(type, valid_until);",
             "CREATE INDEX IF NOT EXISTS idx_memory_superseded_by ON memory(superseded_by);",
             "CREATE INDEX IF NOT EXISTS idx_memory_scope ON memory(scope);" }) {
        if (!storage.executeNonQuery(idx)) {
            std::cerr << "Warning: Failed to create index: " << idx << std::endl;
        }
    }

    return true;
}
