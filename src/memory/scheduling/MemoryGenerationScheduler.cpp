#include "MemoryGenerationScheduler.h"
#include "../generation/MemoryGenerationManager.h"
#include "../../database/managers/MemoryOrchestratorImpl.h"
#include "../../database/core/ORMWrapper.h"
#include "../../database/initialization/ConfigManager.h"
#include "../../models/EmbeddingIndexManager.h"
#include <iostream>
#include <memory>
#include <thread>
#include <algorithm>
#include <unordered_set>

std::chrono::steady_clock::time_point MemoryGenerationScheduler::last_check_time_ = std::chrono::steady_clock::now();
std::chrono::seconds MemoryGenerationScheduler::check_interval_ = std::chrono::seconds(300);
bool MemoryGenerationScheduler::enabled_ = true;
int MemoryGenerationScheduler::consistency_check_counter_ = 0;

void MemoryGenerationScheduler::periodicCheck(MemoryOrchestrator& orchestrator) {
    // 从配置读取调度开关和周期
    ConfigManager& config = ConfigManager::getInstance();
    enabled_ = config.getMemorySchedulerEnabled();
    int interval_seconds = config.getMemorySchedulerIntervalSeconds();
    if (interval_seconds <= 0) {
        interval_seconds = 300;
    }
    check_interval_ = std::chrono::seconds(interval_seconds);
    
    if (!enabled_) {
        std::cout << "[MemoryGenerationScheduler] Scheduler disabled by config, skip check" << std::endl;
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now - last_check_time_ >= check_interval_) {
        std::cout << "[MemoryGenerationScheduler] Starting periodic check (interval="
                  << interval_seconds << "s)" << std::endl;
        
        // 获取所有需要检查的用户
        auto active_users = getActiveUsers(orchestrator);
        if (active_users.empty()) {
            std::cout << "[MemoryGenerationScheduler] No active users with unprocessed messages" << std::endl;
        } else {
            std::cout << "[MemoryGenerationScheduler] Active users: ";
            for (std::size_t i = 0; i < active_users.size(); ++i) {
                std::cout << active_users[i];
                if (i + 1 < active_users.size()) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        std::vector<int> triggered_users;
        
        for (int user_id : active_users) {
            // 使用dynamic_cast获取具体实现
            MemoryOrchestratorImpl* impl = dynamic_cast<MemoryOrchestratorImpl*>(&orchestrator);
            if (impl) {
                // 创建指向具体实现的shared_ptr，但不接管所有权
                auto orchestratorPtr = std::shared_ptr<MemoryOrchestratorImpl>(impl, [](MemoryOrchestratorImpl*){ /* noop deleter */ });
                MemoryGenerationManager manager(orchestratorPtr);
                
                if (manager.shouldGenerateMemories(user_id)) {
                    std::cout << "[MemoryGenerationScheduler] Triggering memory generation for user " 
                              << user_id << std::endl;
                    manager.processUnprocessedMessages(user_id);
                    triggered_users.push_back(user_id);
                } else {
                    std::cout << "[MemoryGenerationScheduler] Skip user " << user_id << " (not enough sources)" << std::endl;
                }
            } else {
                std::cerr << "[MemoryGenerationScheduler] Error: Cannot process abstract orchestrator instance" << std::endl;
                break;
            }
        }
        
        if (triggered_users.empty()) {
            std::cout << "[MemoryGenerationScheduler] No users triggered memory generation this round" << std::endl;
        } else {
            std::cout << "[MemoryGenerationScheduler] Users triggered this round: ";
            for (std::size_t i = 0; i < triggered_users.size(); ++i) {
                std::cout << triggered_users[i];
                if (i + 1 < triggered_users.size()) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        last_check_time_ = now;
        
        // 周期性索引一致性检查：每10次调度检查一次
        consistency_check_counter_++;
        if (consistency_check_counter_ >= 10) {
            consistency_check_counter_ = 0;
            std::cout << "[MemoryGenerationScheduler] Running periodic embedding index consistency check..." << std::endl;
            
            try {
                EmbeddingIndexManager embeddingMgr;
                std::string db_path = config.getDatabasePath();
                
                // 对所有活跃用户进行一致性检查
                for (int user_id : active_users) {
                    auto report = embeddingMgr.checkIndexConsistency(user_id, db_path);
                    
                    if (report.orphaned_embeddings > 0) {
                        std::cout << "[MemoryGenerationScheduler] User " << user_id 
                                  << ": Found " << report.orphaned_embeddings << " orphaned embeddings" << std::endl;
                        
                        // 自动清理孤儿 embedding
                        if (!report.orphaned_ids.empty()) {
                            embeddingMgr.cleanOrphanedEmbeddings(report.orphaned_ids, db_path);
                        }
                    } else {
                        std::cout << "[MemoryGenerationScheduler] User " << user_id 
                                  << ": Index consistency OK" << std::endl;
                    }
                }
                
                std::cout << "[MemoryGenerationScheduler] Consistency check completed" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[MemoryGenerationScheduler] Consistency check failed: " << e.what() << std::endl;
            }
        }
        
        std::cout << "[MemoryGenerationScheduler] Periodic check completed" << std::endl;
    }
}

std::vector<int> MemoryGenerationScheduler::getActiveUsers(MemoryOrchestrator& orchestrator) {
    std::vector<int> active_users;
    
    // 尝试基于未处理消息动态选择最近活跃用户
    try {
        auto* impl = dynamic_cast<MemoryOrchestratorImpl*>(&orchestrator);
        if (impl) {
            ORMWrapper& orm = impl->getORMWrapper();
            auto all_messages = orm.getAll<MessageMemory>();
            if (!all_messages.empty()) {
                // 按id从大到小排序，近似按时间从新到旧
                std::sort(all_messages.begin(), all_messages.end(), [](const MessageMemory& a, const MessageMemory& b) {
                    return a.id > b.id;
                });
                
                std::unordered_set<int> user_set;
                const std::size_t max_users = 10; // 简单上限控制
                for (const auto& msg : all_messages) {
                    // 只考虑尚未处理的消息，近似表示有待生成长期记忆
                    if (!msg.processed) {
                        if (user_set.insert(msg.user_id).second) {
                            active_users.push_back(msg.user_id);
                            if (active_users.size() >= max_users) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[MemoryGenerationScheduler] getActiveUsers failed: " << e.what() << std::endl;
    }
    
    // 回退：如果无法基于数据确定活跃用户，则保留原有默认行为
    if (active_users.empty()) {
        active_users.push_back(1);
    }
    
    return active_users;
}

bool MemoryGenerationScheduler::isSessionEnded(int user_id, MemoryOrchestrator& orchestrator) {
    // 简化实现，实际应用中应该检查会话状态
    return true; // 假设会话已结束，触发记忆生成
}