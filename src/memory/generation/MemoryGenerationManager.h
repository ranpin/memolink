#ifndef MEMORY_GENERATION_MANAGER_H
#define MEMORY_GENERATION_MANAGER_H

#include "../../database/managers/MemoryOrchestrator.h"
#include <chrono>
#include <memory>
#include <vector>
#include <string>

// 包含各个生成器的头文件
#include "generators/EpisodicMemoryGenerator.h"
#include "generators/SemanticMemoryGenerator.h"
#include "generators/CoreMemoryGenerator.h"
#include "generators/ProceduralMemoryGenerator.h"
#include "generators/SecretVaultGenerator.h"

class ORMWrapper;

class MemoryGenerationManager {
public:
    struct MemoryGenerationConfig {
        int message_threshold = 5;           // 消息数量/原材料数量阈值
        int time_interval_minutes = 30;      // 时间间隔（分钟）
        float confidence_threshold = 0.5f;   // 置信度阈值
        int max_sources_per_generation = 50; // 单次生成中最多使用的来源数量（消息+多模态）
        bool enable_provenance = true;       // 是否写入溯源记录
        bool enable_episodic = true;
        bool enable_semantic = true;
        bool enable_core = true;
        bool enable_procedural = true;
        bool enable_secret_vault = true;
        
        // 溯源范围细化：类型配额
        int max_messages_quota = 10;         // 每次生成记忆时，最多收集的消息数量
        int max_screenshots_quota = 5;       // 最多收集的截图数量
        int max_audios_quota = 3;            // 最多收集的音频数量
        int max_images_quota = 5;            // 最多收集的图像数量
        bool filter_by_current_session = false; // 是否仅从当前会话收集来源
    };

    // 统一的记忆来源抽象：同时覆盖消息与多模态数据
    struct MemorySource {
        enum class SourceCategory {
            Message,
            Multimodal
        };

        SourceCategory source_category;   // 来源类别
        int source_id = 0;               // 在对应源表中的ID（message_memories / multimodal_memories）
        int user_id = 0;
        int agent_id = 0;
        int session_id = 0;
        std::string modality;            // "text" / "image" / "audio" / "screenshot" 等
        std::string text;                // 消息内容或多模态描述文本
        std::string file_path;           // 多模态文件路径（文本消息为空）
        std::string metadata;            // 预留的元数据
        std::string created_at;          // 创建时间
    };

    MemoryGenerationManager(std::shared_ptr<MemoryOrchestrator> orchestrator);
    bool shouldGenerateMemories(int user_id);
    void processUnprocessedMessages(int user_id);
    
    // 配置管理
    void setConfig(const MemoryGenerationConfig& config) { config_ = config; }
    MemoryGenerationConfig getConfig() { return config_; }

private:
    std::shared_ptr<MemoryOrchestrator> orchestrator_;
    
    MemoryGenerationConfig config_;
    
    std::chrono::system_clock::time_point getLastMemoryGenerationTime(int user_id);
    bool isSessionEnded(int user_id);
    int getMessageCountSinceLastMemory(int user_id);
    void updateLastGenerationTime(int user_id);
    
    // 私有辅助方法
    std::string getCurrentTimestamp();

    // 统一从 message_memories 与 multimodal_memories 收集原材料
    std::vector<MemorySource> collectSources(int user_id, ORMWrapper& orm, int max_sources);
    
public:
    // 保存各种类型记忆的公共方法（使用静态生成器输出）
    void saveEpisodicMemory(const EpisodicMemoryGenerator::GeneratedEpisodicMemory& memory, 
                           int user_id, int agent_id);
    void saveSemanticMemory(const SemanticMemoryGenerator::GeneratedSemanticMemory& memory, 
                           int user_id, int agent_id);
    void saveCoreMemory(const CoreMemoryGenerator::GeneratedCoreMemory& memory, 
                       int user_id, int agent_id);
    void saveProceduralMemory(const ProceduralMemoryGenerator::GeneratedProceduralMemory& memory, 
                             int user_id, int agent_id);
    void saveSecretVault(const SecretVaultGenerator::GeneratedSecretVault& memory, 
                           int user_id, int agent_id);
};

#endif // MEMORY_GENERATION_MANAGER_H