#include <iostream>
#include "../src/models/EmbeddingIndexManager.h"
#include "../src/database/initialization/ConfigManager.h"

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Embedding 索引一致性检查工具" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    try {
        // 获取数据库路径
        ConfigManager& config = ConfigManager::getInstance();
        std::string db_path = config.getDatabasePath();
        
        std::cout << "[检查工具] 数据库路径: " << db_path << "\n" << std::endl;
        
        // 创建 EmbeddingIndexManager
        EmbeddingIndexManager manager;
        
        // 执行一致性检查（userId = 1）
        std::cout << "[检查工具] 开始检查用户 1 的 embedding 索引...\n" << std::endl;
        auto report = manager.checkIndexConsistency(1, db_path);
        
        // 输出检查报告
        std::cout << "\n========================================" << std::endl;
        std::cout << "  检查报告汇总" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "总 embedding 数量     : " << report.total_embeddings << std::endl;
        std::cout << "有效 embedding        : " << report.valid_embeddings << std::endl;
        std::cout << "孤儿 embedding        : " << report.orphaned_embeddings << std::endl;
        std::cout << "用户ID不匹配          : " << report.user_mismatch << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        // 如果有孤儿 embedding，询问是否清理
        if (!report.orphaned_ids.empty()) {
            std::cout << "[检查工具] 发现 " << report.orphaned_ids.size() << " 条孤儿 embedding\n" << std::endl;
            std::cout << "是否清理这些孤儿 embedding? (y/n): ";
            
            char choice;
            std::cin >> choice;
            
            if (choice == 'y' || choice == 'Y') {
                bool success = manager.cleanOrphanedEmbeddings(report.orphaned_ids, db_path);
                if (success) {
                    std::cout << "\n✓ 清理成功！" << std::endl;
                    
                    // 再次检查验证结果
                    std::cout << "\n[检查工具] 重新检查以验证清理结果...\n" << std::endl;
                    auto verify_report = manager.checkIndexConsistency(1, db_path);
                    
                    std::cout << "\n========================================" << std::endl;
                    std::cout << "  清理后验证报告" << std::endl;
                    std::cout << "========================================" << std::endl;
                    std::cout << "总 embedding 数量     : " << verify_report.total_embeddings << std::endl;
                    std::cout << "有效 embedding        : " << verify_report.valid_embeddings << std::endl;
                    std::cout << "孤儿 embedding        : " << verify_report.orphaned_embeddings << std::endl;
                    std::cout << "========================================\n" << std::endl;
                    
                    if (verify_report.orphaned_embeddings == 0) {
                        std::cout << "✓ 所有孤儿 embedding 已清理完成！" << std::endl;
                    }
                } else {
                    std::cerr << "\n✗ 清理失败" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "\n[检查工具] 取消清理操作" << std::endl;
            }
        } else {
            std::cout << "✓ 没有发现孤儿 embedding，索引状态良好！" << std::endl;
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  检查工具执行完成" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ 错误: " << e.what() << std::endl;
        return 1;
    }
}
