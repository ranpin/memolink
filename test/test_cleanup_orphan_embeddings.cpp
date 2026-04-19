#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <string>

int main() {
    std::cout << "\n========== 孤儿 Embedding 清理工具 ==========\n" << std::endl;
    
    // 直接操作数据库
    std::string db_path = "/Users/ranpin/code/mirix-cpp/mirix.db";
    std::cout << "数据库路径: " << db_path << "\n" << std::endl;
    
    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "错误: 无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    
    // 步骤1: 统计孤儿 embedding
    std::cout << "========== 步骤 1: 扫描孤儿 embedding ==========\n" << std::endl;
    
    const char* countSQL = R"(
        SELECT COUNT(*) 
        FROM embedding_index e
        WHERE NOT EXISTS (
            SELECT 1 FROM episodic_memories WHERE id = e.memory_id AND e.memory_type = 'episodic'
            UNION
            SELECT 1 FROM semantic_memories WHERE id = e.memory_id AND e.memory_type = 'semantic'
            UNION
            SELECT 1 FROM core_memory_blocks WHERE id = e.memory_id AND e.memory_type = 'core'
            UNION
            SELECT 1 FROM procedural_memories WHERE id = e.memory_id AND e.memory_type = 'procedural'
            UNION
            SELECT 1 FROM multimodal_memories WHERE id = e.memory_id AND e.memory_type IN ('screenshot', 'image', 'audio')
        )
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int orphan_count = 0;
    
    if (sqlite3_prepare_v2(db, countSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            orphan_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    std::cout << "发现孤儿 embedding: " << orphan_count << " 条" << std::endl;
    
    if (orphan_count == 0) {
        std::cout << "\n✓ 没有发现孤儿 embedding，索引状态良好！" << std::endl;
        sqlite3_close(db);
        return 0;
    }
    
    // 步骤2: 清理孤儿 embedding
    std::cout << "\n========== 步骤 2: 清理孤儿 embedding ==========\n" << std::endl;
    
    char confirm;
    std::cout << "确认清理 " << orphan_count << " 条孤儿 embedding? (y/n): ";
    std::cin >> confirm;
    
    if (confirm != 'y' && confirm != 'Y') {
        std::cout << "\n取消清理操作" << std::endl;
        sqlite3_close(db);
        return 0;
    }
    
    const char* deleteSQL = R"(
        DELETE FROM embedding_index
        WHERE id IN (
            SELECT e.id
            FROM embedding_index e
            WHERE NOT EXISTS (
                SELECT 1 FROM episodic_memories WHERE id = e.memory_id AND e.memory_type = 'episodic'
                UNION
                SELECT 1 FROM semantic_memories WHERE id = e.memory_id AND e.memory_type = 'semantic'
                UNION
                SELECT 1 FROM core_memory_blocks WHERE id = e.memory_id AND e.memory_type = 'core'
                UNION
                SELECT 1 FROM procedural_memories WHERE id = e.memory_id AND e.memory_type = 'procedural'
                UNION
                SELECT 1 FROM multimodal_memories WHERE id = e.memory_id AND e.memory_type IN ('screenshot', 'image', 'audio')
            )
        )
    )";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, deleteSQL, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "错误: 清理失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }
    
    int deleted_count = sqlite3_changes(db);
    std::cout << "已成功清理: " << deleted_count << " 条 embedding" << std::endl;
    
    // 步骤3: 验证清理结果
    std::cout << "\n========== 步骤 3: 验证清理结果 ==========\n" << std::endl;
    
    int remaining = 0;
    if (sqlite3_prepare_v2(db, countSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            remaining = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    std::cout << "清理前后对比:" << std::endl;
    std::cout << "  孤儿 embedding: " << orphan_count << " -> " << remaining << std::endl;
    
    if (remaining == 0) {
        std::cout << "\n✓ 所有孤儿 embedding 已清理完毕！" << std::endl;
    } else {
        std::cout << "\n⚠ 还有 " << remaining << " 条孤儿 embedding 未清理" << std::endl;
    }
    
    sqlite3_close(db);
    std::cout << "\n========== 清理工具执行完成 ==========\n" << std::endl;
    
    return 0;
}
