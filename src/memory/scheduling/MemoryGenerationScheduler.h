#ifndef MEMORY_GENERATION_SCHEDULER_H
#define MEMORY_GENERATION_SCHEDULER_H

#include "../../database/managers/MemoryOrchestrator.h"
#include <chrono>
#include <vector>

class MemoryGenerationScheduler {
private:
    static std::chrono::steady_clock::time_point last_check_time_;
    static std::chrono::seconds check_interval_;
    static bool enabled_;
    static int consistency_check_counter_;  // 用于控制一致性检查频率
    
public:
    static void periodicCheck(MemoryOrchestrator& orchestrator);
    
private:
    static std::vector<int> getActiveUsers(MemoryOrchestrator& orchestrator);
    static bool isSessionEnded(int user_id, MemoryOrchestrator& orchestrator);
};

#endif // MEMORY_GENERATION_SCHEDULER_H