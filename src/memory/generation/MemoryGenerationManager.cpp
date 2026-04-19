#include "MemoryGenerationManager.h"
#include "generators/EpisodicMemoryGenerator.h"
#include "generators/SemanticMemoryGenerator.h"
#include "generators/CoreMemoryGenerator.h"
#include "generators/ProceduralMemoryGenerator.h"
#include "generators/SecretVaultGenerator.h"
#include "../messages/MessageManager.h"
#include "../../database/models/MemoryModels.h"
#include "../../models/QwenAPI.h"
#include "../../database/core/ORMWrapper.h"
#include "../../database/core/StorageManager.h"
#include "../../database/managers/MemoryProvenanceManager.h"
#include "../../database/managers/EpisodicMemoryManager.h"
#include "../../database/managers/SemanticMemoryManager.h"
#include "../../database/managers/CoreMemoryBlockManager.h"
#include "../../database/managers/ProceduralMemoryManager.h"
#include "../../database/managers/SecretVaultManager.h"
#include "../../database/initialization/ConfigManager.h"
#include "../../models/TextEmbeddingGenerator.h"
#include "../../models/EmbeddingIndexManager.h"
#include "../../utils/TimeUtils.h"
#include <json/json.h>
#include <iostream>
#include <chrono>

MemoryGenerationManager::MemoryGenerationManager(std::shared_ptr<MemoryOrchestrator> orchestrator) 
    : orchestrator_(orchestrator) {
    // 从配置文件加载配置
    ConfigManager& configManager = ConfigManager::getInstance();
    
    config_.message_threshold = configManager.getMemoryGenerationMessageThreshold();
    config_.time_interval_minutes = configManager.getMemoryGenerationTimeIntervalMinutes();
    config_.confidence_threshold = configManager.getMemoryGenerationConfidenceThreshold();
    config_.max_sources_per_generation = configManager.getMemoryGenerationMaxSourcesPerGeneration();
    config_.enable_provenance = configManager.getMemoryGenerationEnableProvenance();
    config_.enable_episodic = configManager.getMemoryGenerationEnableEpisodic();
    config_.enable_semantic = configManager.getMemoryGenerationEnableSemantic();
    config_.enable_core = configManager.getMemoryGenerationEnableCore();
    config_.enable_procedural = configManager.getMemoryGenerationEnableProcedural();
    config_.enable_secret_vault = configManager.getMemoryGenerationEnableSecretVault();
    
    // 溯源范围细化配置
    config_.max_messages_quota = configManager.getMemoryGenerationMaxMessagesQuota();
    config_.max_screenshots_quota = configManager.getMemoryGenerationMaxScreenshotsQuota();
    config_.max_audios_quota = configManager.getMemoryGenerationMaxAudiosQuota();
    config_.max_images_quota = configManager.getMemoryGenerationMaxImagesQuota();
    config_.filter_by_current_session = configManager.getMemoryGenerationFilterByCurrentSession();
    
    std::cout << "[MemoryGenerationManager] 已从配置文件加载配置:" << std::endl;
    std::cout << "  消息阈值: " << config_.message_threshold << std::endl;
    std::cout << "  时间间隔(分钟): " << config_.time_interval_minutes << std::endl;
    std::cout << "  置信度阈值: " << config_.confidence_threshold << std::endl;
    std::cout << "  最大来源数量: " << config_.max_sources_per_generation << std::endl;
    std::cout << "  启用溯源: " << (config_.enable_provenance ? "是" : "否") << std::endl;
    std::cout << "  启用情景记忆: " << (config_.enable_episodic ? "是" : "否") << std::endl;
    std::cout << "  启用语义记忆: " << (config_.enable_semantic ? "是" : "否") << std::endl;
    std::cout << "  启用核心记忆: " << (config_.enable_core ? "是" : "否") << std::endl;
    std::cout << "  启用程序记忆: " << (config_.enable_procedural ? "是" : "否") << std::endl;
    std::cout << "  启用秘密保险库: " << (config_.enable_secret_vault ? "是" : "否") << std::endl;
    std::cout << "  消息配额: " << config_.max_messages_quota << std::endl;
    std::cout << "  截图配额: " << config_.max_screenshots_quota << std::endl;
    std::cout << "  音频配额: " << config_.max_audios_quota << std::endl;
    std::cout << "  图像配额: " << config_.max_images_quota << std::endl;
    std::cout << "  仅当前会话: " << (config_.filter_by_current_session ? "是" : "否") << std::endl;
}

bool MemoryGenerationManager::shouldGenerateMemories(int user_id) {
    // 统一使用“原材料数量”（消息 + 多模态）作为阈值判断依据
    ORMWrapper& orm = orchestrator_->getORMWrapper();
    auto sources = collectSources(user_id, orm, config_.max_sources_per_generation);
    int totalCount = static_cast<int>(sources.size());

    std::cout << "[MemoryGenerationManager] 检查是否需要生成记忆:" << std::endl;
    std::cout << "  用户ID: " << user_id << std::endl;
    std::cout << "  原材料总数(消息+多模态): " << totalCount << std::endl;
    std::cout << "  数量阈值: " << config_.message_threshold << std::endl;
    
    if (totalCount >= config_.message_threshold) {
        std::cout << "  => 达到数量阈值，需要生成记忆" << std::endl;
        return true;
    }

    std::cout << "  => 未达到数量阈值，暂不生成记忆" << std::endl;
    return false;
}

void MemoryGenerationManager::processUnprocessedMessages(int user_id) {
    auto start = std::chrono::steady_clock::now();
    std::cout << "\n========================================" << std::endl;
    std::cout << "[MemoryGenerationManager] 开始处理未处理消息" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 获取未处理的消息
    auto messages = MessageManager::getUnprocessedMessages(user_id, *orchestrator_);
    std::cout << "[MemoryGenerationManager] 获取到 " << messages.size() << " 条未处理消息" << std::endl;

    // 收集统一的记忆来源（消息 + 多模态）
    ORMWrapper& orm = orchestrator_->getORMWrapper();
    auto sources = collectSources(user_id, orm, config_.max_sources_per_generation);
    
    int processed_count = 0;
    int success_count = 0;
    
    // 溯源管理器
    MemoryProvenanceManager provenanceManager(orm);
    EpisodicMemoryManager& episodicManager = orchestrator_->getEpisodicMemoryManager();
    SemanticMemoryManager& semanticManager = orchestrator_->getSemanticMemoryManager();
    CoreMemoryBlockManager& coreManager = orchestrator_->getCoreMemoryBlockManager();
    ProceduralMemoryManager& proceduralManager = orchestrator_->getProceduralMemoryManager();
    SecretVaultManager& secretManager = orchestrator_->getSecretVaultManager();

    auto recordProvenance = [&](const std::string& memory_type, int memory_id) {
        if (memory_id <= 0) return;
        for (const auto& src : sources) {
            MemoryProvenance p;
            p.memory_type = memory_type;
            p.memory_id = memory_id;
            p.source_type = (src.source_category == MemorySource::SourceCategory::Message) ? "message" : "multimodal";
            p.source_id = src.source_id;
            p.relation_type = "evidence";
            p.created_at = TimeUtils::getCurrentTimestamp();
            provenanceManager.add(p);
        }
    };
    
    // Embedding生成与索引更新函数：从内容提取文本 -> 生成embedding -> 写入索引
    auto generateAndIndexEmbedding = [&](const std::string& memory_type, int memory_id, const std::string& content_text) {
        if (memory_id <= 0 || content_text.empty()) {
            std::cout << "[MemoryGenerationManager] Embedding生成跳过: memory_id=" << memory_id 
                      << ", content_text长度=" << content_text.length() << std::endl;
            return;
        }
        
        try {
            std::cout << "[MemoryGenerationManager] 开始为 " << memory_type << " ID=" << memory_id 
                      << " 生成embedding..." << std::endl;
            
            // 创建 StorageManager 用于 TextEmbeddingGenerator
            ConfigManager& cfg = ConfigManager::getInstance();
            std::string db_path = cfg.getDatabasePath();
            if (db_path.empty()) {
                db_path = "mirix.db";
            }
            StorageManager storage(db_path);
            storage.initialize();
            
            // 使用 TextEmbeddingGenerator 生成 embedding
            TextEmbeddingGenerator embeddingGen(&storage);
            std::vector<float> embedding = embeddingGen.generateTextEmbedding(content_text);
            
            if (embedding.empty()) {
                std::cout << "[MemoryGenerationManager] ⚠ Embedding生成返回空向量" << std::endl;
                return;
            }
            
            std::cout << "[MemoryGenerationManager] ✓ Embedding生成成功, 维度: " << embedding.size() << std::endl;
            
            // 构造metadata JSON：包含 user_id, timestamp, importance 等
            Json::Value meta;
            meta["user_id"] = user_id;
            meta["timestamp"] = TimeUtils::getCurrentTimestamp();
            meta["memory_type"] = memory_type;
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string metadata_str = Json::writeString(builder, meta);
            
            // 写入 EmbeddingIndexManager
            EmbeddingIndexManager embeddingIndex;
            bool indexSuccess = embeddingIndex.addEmbedding(memory_id, memory_type, embedding, metadata_str, db_path);
            
            if (indexSuccess) {
                std::cout << "[MemoryGenerationManager] ✓ Embedding索引更新成功" << std::endl;
            } else {
                std::cout << "[MemoryGenerationManager] ✗ Embedding索引更新失败" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[MemoryGenerationManager] Embedding生成/索引异常: " << e.what() << std::endl;
        }
    };
    
    // 为多模态来源生成embedding，使用data_type作为memory_type，source_id作为memory_id
    for (const auto& src : sources) {
        if (src.source_category == MemorySource::SourceCategory::Multimodal && !src.text.empty()) {
            generateAndIndexEmbedding(src.modality, src.source_id, src.text);
        }
    }
    
    for (const auto& message : messages) {
        processed_count++;
        std::cout << "\n--- 处理消息 " << processed_count << "/" << messages.size() << " ---" << std::endl;
        std::cout << "消息ID: " << message.id << std::endl;
        std::cout << "消息内容(前100字符): " << message.content.substr(0, std::min(100, (int)message.content.length())) << "..." << std::endl;
        
        // 使用事务包裹整个记忆生成流程：分类 -> 生成 -> 溯源 -> 标记
        bool transaction_success = false;
        try {
            orm.beginTransaction();
            std::cout << "[MemoryGenerationManager] 开始事务..." << std::endl;
        
        // 构造分类提示词
        std::string classification_prompt = R"(
请根据以下对话内容，判断应该生成哪种类型的记忆，并给出置信度分数（0-1之间）：
1. 情景记忆(Episodic): 个人经历、事件、时间地点等具体情境
2. 语义记忆(Semantic): 事实、概念、知识等抽象信息
3. 核心记忆(Core): 个人核心信念、价值观、身份认同
4. 程序记忆(Procedural): 技能、习惯、操作步骤
5. 资源记忆(Resource): 有用的链接、工具、参考资料
6. 秘密保险库(SecretVault): 隐私或敏感信息

对话内容: )" + message.content + R"(

请按照以下JSON格式回复：
{
    "type": "数字编号",
    "confidence": "置信度分数"
}
)";
        
        std::cout << "[MemoryGenerationManager] 调用分类模型..." << std::endl;
        
        // 调用Qwen API进行分类
        QwenAPI qwen;
        QwenChatCompletionRequest request;
        request.model = "qwen-plus";
        QwenMessage qwen_msg;
        qwen_msg.role = "user";
        qwen_msg.content = classification_prompt;
        request.messages.push_back(qwen_msg);
        
        QwenChatCompletionResponse response = qwen.chat_completion(request);
        std::cout << "[MemoryGenerationManager] 分类结果: " << response.content << std::endl;
        
        // 解析分类结果
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(response.content, root)) {
            int max_type = root["type"].asInt();
            float max_confidence = root["confidence"].asFloat();
            
            std::cout << "[MemoryGenerationManager] 记忆类型: " << max_type << ", 置信度: " << max_confidence << std::endl;
            
            const char* type_names[] = {"未知", "情景记忆", "语义记忆", "核心记忆", "程序记忆", "秘密保险库"};
            if (max_type >= 1 && max_type <= 5) {
                std::cout << "[MemoryGenerationManager] => " << type_names[max_type] << std::endl;
            }
            
            bool generation_success = false;
            int generated_memory_id = -1;
            
            // 检查该类型在当前配置下是否启用
            bool type_enabled = false;
            switch (max_type) {
                case 1:
                    type_enabled = config_.enable_episodic;
                    break;
                case 2:
                    type_enabled = config_.enable_semantic;
                    break;
                case 3:
                    type_enabled = config_.enable_core;
                    break;
                case 4:
                    type_enabled = config_.enable_procedural;
                    break;
                case 5:
                    type_enabled = config_.enable_secret_vault;
                    break;
                default:
                    break;
            }
            
            // 如果配置启用了该类型且置信度足够高，则生成相应类型的记忆
            if (!type_enabled) {
                std::cout << "[MemoryGenerationManager] 当前配置已禁用该类型记忆生成，跳过生成" << std::endl;
            } else if (max_confidence >= config_.confidence_threshold) {
                std::cout << "[MemoryGenerationManager] 置信度超过阈值(" << config_.confidence_threshold << ")，开始生成记忆..." << std::endl;
                
                switch (max_type) {
                    case 1: { // Episodic
                        std::cout << "[MemoryGenerationManager] 调用情景记忆生成器..." << std::endl;
                        EpisodicMemoryGenerator episodicGenerator;
                        generation_success = episodicGenerator.processAndSave({message}, message.user_id, message.agent_id, message.session_id, *orchestrator_);
                        if (generation_success) {
                            // 获取当前会话下最新的情景记忆ID
                            auto episodics = episodicManager.getBySessionId(message.session_id);
                            int maxId = -1;
                            std::string content_for_embedding;
                            for (const auto& em : episodics) {
                                if (em.id > maxId) {
                                    maxId = em.id;
                                    content_for_embedding = em.event_description;
                                }
                            }
                            generated_memory_id = maxId;
                            std::cout << "[MemoryGenerationManager] ✓ 情景记忆生成成功, ID=" << generated_memory_id << std::endl;
                            
                            // 生成并索引 embedding
                            generateAndIndexEmbedding("episodic", generated_memory_id, content_for_embedding);
                        } else {
                            std::cout << "[MemoryGenerationManager] ✗ 情景记忆生成失败" << std::endl;
                        }
                        break;
                    }
                    case 2: { // Semantic
                        std::cout << "[MemoryGenerationManager] 调用语义记忆生成器..." << std::endl;
                        SemanticMemoryGenerator semanticGenerator;
                        generation_success = semanticGenerator.processAndSave({message}, message.user_id, message.agent_id, message.session_id, *orchestrator_);
                        if (generation_success) {
                            auto semantics = semanticManager.getByUserId(message.user_id);
                            int maxId = -1;
                            std::string content_for_embedding;
                            for (const auto& sm : semantics) {
                                if (sm.id > maxId) {
                                    maxId = sm.id;
                                    content_for_embedding = sm.fact;
                                }
                            }
                            generated_memory_id = maxId;
                            std::cout << "[MemoryGenerationManager] ✓ 语义记忆生成成功, ID=" << generated_memory_id << std::endl;
                            
                            // 生成并索引 embedding
                            generateAndIndexEmbedding("semantic", generated_memory_id, content_for_embedding);
                        } else {
                            std::cout << "[MemoryGenerationManager] ✗ 语义记忆生成失败" << std::endl;
                        }
                        break;
                    }
                    case 3: { // Core
                        std::cout << "[MemoryGenerationManager] 调用核心记忆生成器..." << std::endl;
                        CoreMemoryGenerator coreGenerator;
                        generation_success = coreGenerator.processAndSave({message}, message.user_id, message.agent_id, message.session_id, *orchestrator_);
                        if (generation_success) {
                            auto cores = coreManager.getByUserId(message.user_id);
                            int maxId = -1;
                            std::string content_for_embedding;
                            for (const auto& cm : cores) {
                                if (cm.id > maxId) {
                                    maxId = cm.id;
                                    content_for_embedding = cm.content;
                                }
                            }
                            generated_memory_id = maxId;
                            std::cout << "[MemoryGenerationManager] ✓ 核心记忆生成成功, ID=" << generated_memory_id << std::endl;
                            
                            // 生成并索引 embedding
                            generateAndIndexEmbedding("core", generated_memory_id, content_for_embedding);
                        } else {
                            std::cout << "[MemoryGenerationManager] ✗ 核心记忆生成失败" << std::endl;
                        }
                        break;
                    }
                    case 4: { // Procedural
                        std::cout << "[MemoryGenerationManager] 调用程序记忆生成器..." << std::endl;
                        ProceduralMemoryGenerator proceduralGenerator;
                        generation_success = proceduralGenerator.processAndSave({message}, message.user_id, message.agent_id, message.session_id, *orchestrator_);
                        if (generation_success) {
                            auto procs = proceduralManager.getByUserId(message.user_id);
                            int maxId = -1;
                            std::string content_for_embedding;
                            for (const auto& pm : procs) {
                                if (pm.id > maxId) {
                                    maxId = pm.id;
                                    content_for_embedding = pm.description;
                                }
                            }
                            generated_memory_id = maxId;
                            std::cout << "[MemoryGenerationManager] ✓ 程序记忆生成成功, ID=" << generated_memory_id << std::endl;
                            
                            // 生成并索引 embedding
                            generateAndIndexEmbedding("procedural", generated_memory_id, content_for_embedding);
                        } else {
                            std::cout << "[MemoryGenerationManager] ✗ 程序记忆生成失败" << std::endl;
                        }
                        break;
                    }
                    case 5: { // SecretVault
                        std::cout << "[MemoryGenerationManager] 调用秘密保险库生成器..." << std::endl;
                        SecretVaultGenerator secretGenerator;
                        generation_success = secretGenerator.processAndSave({message}, message.user_id, message.agent_id, message.session_id, *orchestrator_);
                        if (generation_success) {
                            auto secrets = secretManager.getByUserId(message.user_id);
                            int maxId = -1;
                            std::string content_for_embedding;
                            for (const auto& sv : secrets) {
                                if (sv.id > maxId) {
                                    maxId = sv.id;
                                    content_for_embedding = sv.content;
                                }
                            }
                            generated_memory_id = maxId;
                            std::cout << "[MemoryGenerationManager] ✓ 秘密保险库生成成功, ID=" << generated_memory_id << std::endl;
                            
                            // 生成并索引 embedding
                            generateAndIndexEmbedding("secret_vault", generated_memory_id, content_for_embedding);
                        } else {
                            std::cout << "[MemoryGenerationManager] ✗ 秘密保险库生成失败" << std::endl;
                        }
                        break;
                    }
                    default:
                        std::cout << "[MemoryGenerationManager] 未知的记忆类型: " << max_type << std::endl;
                        break;
                }
            } else {
                std::cout << "[MemoryGenerationManager] 置信度低于阈值(" << config_.confidence_threshold << ")，跳过记忆生成" << std::endl;
            }
            
            if (generation_success) {
                // 标记消息为已处理
                MessageManager::markMessagesAsProcessed({message.id}, *orchestrator_);
                success_count++;
                std::cout << "[MemoryGenerationManager] 消息已标记为已处理" << std::endl;
                
                // 对所有记忆类型写入溯源记录
                if (config_.enable_provenance && generated_memory_id > 0) {
                    if (max_type == 1) {
                        recordProvenance("episodic", generated_memory_id);
                    } else if (max_type == 2) {
                        recordProvenance("semantic", generated_memory_id);
                    } else if (max_type == 3) {
                        recordProvenance("core", generated_memory_id);
                    } else if (max_type == 4) {
                        recordProvenance("procedural", generated_memory_id);
                    } else if (max_type == 5) {
                        recordProvenance("secret_vault", generated_memory_id);
                    }
                }
                
                // 事务提交
                orm.commit();
                transaction_success = true;
                std::cout << "[MemoryGenerationManager] 事务提交成功" << std::endl;
            } else {
                // 生成失败，回滚事务
                orm.rollback();
                std::cout << "[MemoryGenerationManager] 记忆生成失败，事务已回滚" << std::endl;
            }
        } else {
            std::cout << "[MemoryGenerationManager] ✗ 解析分类结果失败" << std::endl;
            orm.rollback();
            std::cout << "[MemoryGenerationManager] 事务已回滚" << std::endl;
        }
        
        } catch (const std::exception& e) {
            // 发生异常，回滚事务
            std::cout << "[MemoryGenerationManager] 异常: " << e.what() << std::endl;
            try {
                orm.rollback();
                std::cout << "[MemoryGenerationManager] 事务已回滚" << std::endl;
            } catch (...) {
                std::cout << "[MemoryGenerationManager] 回滚事务失败" << std::endl;
            }
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "[MemoryGenerationManager] 记忆生成完成" << std::endl;
    std::cout << "  处理消息数: " << processed_count << std::endl;
    std::cout << "  成功生成数: " << success_count << std::endl;
    std::cout << "========================================\n" << std::endl;

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[Perf] MemoryGenerationManager::processUnprocessedMessages 完成，耗时 "
              << elapsed_ms << " ms, 处理消息数=" << processed_count
              << "，成功生成记忆数=" << success_count << std::endl;
}

// 以下方法需要在后续实现中完善
std::chrono::system_clock::time_point MemoryGenerationManager::getLastMemoryGenerationTime(int user_id) {
    // 这里应该从数据库中获取上次生成记忆的时间
    return std::chrono::system_clock::time_point(); // 简化实现，返回epoch时间
}

bool MemoryGenerationManager::isSessionEnded(int user_id) {
    // 简化实现，实际应该检查会话状态
    return false;
}

int MemoryGenerationManager::getMessageCountSinceLastMemory(int user_id) {
    return MessageManager::getUnprocessedMessageCount(user_id, *orchestrator_);
}

void MemoryGenerationManager::updateLastGenerationTime(int user_id) {
    // 这里应该更新数据库中的上次生成记忆时间
    std::cout << "[MemoryGenerationManager] Updated last generation time for user " << user_id << std::endl;
}



std::vector<MemoryGenerationManager::MemorySource> MemoryGenerationManager::collectSources(int user_id, ORMWrapper& orm, int max_sources) {
    std::vector<MemorySource> sources;

    std::cout << "[MemoryGenerationManager::collectSources] 开始收集记忆来源:" << std::endl;
    std::cout << "  用户ID: " << user_id << std::endl;
    std::cout << "  最大来源数: " << max_sources << std::endl;
    std::cout << "  消息配额: " << config_.max_messages_quota << std::endl;
    std::cout << "  截图配额: " << config_.max_screenshots_quota << std::endl;
    std::cout << "  音频配额: " << config_.max_audios_quota << std::endl;
    std::cout << "  图像配额: " << config_.max_images_quota << std::endl;
    std::cout << "  仅当前会话: " << (config_.filter_by_current_session ? "是" : "否") << std::endl;

    // 1. 从未处理消息构建文本来源，应用消息配额
    auto messages = MessageManager::getUnprocessedMessages(user_id, *orchestrator_);
    if (!messages.empty()) {
        std::size_t total_messages = messages.size();
        std::size_t msg_limit = static_cast<std::size_t>(config_.max_messages_quota);
        std::size_t start_index = 0;
        
        if (msg_limit > 0 && total_messages > msg_limit) {
            // 只保留最近的 max_messages_quota 条消息
            start_index = total_messages - msg_limit;
        }

        std::cout << "  -> 收集消息: 总数=" << total_messages << ", 起始索引=" << start_index << std::endl;

        for (std::size_t i = start_index; i < total_messages; ++i) {
            const auto& msg = messages[i];
            
            // Session过滤：如果启用，则跳过不属于当前会话的消息
            // （这里简化处理：假定最近的会话就是当前会话）
            // TODO: 可以进一步优化，根据 session_id 过滤
            
            MemorySource src;
            src.source_category = MemorySource::SourceCategory::Message;
            src.source_id = msg.id;
            src.user_id = msg.user_id;
            src.agent_id = msg.agent_id;
            src.session_id = msg.session_id;
            src.modality = "text";
            src.text = msg.content;
            src.file_path.clear();
            src.metadata = msg.metadata;
            src.created_at = msg.created_at;
            sources.push_back(std::move(src));

            // 如果已经达到总来源数量上限，则直接返回
            if (max_sources > 0 && static_cast<int>(sources.size()) >= max_sources) {
                std::cout << "  -> 达到最大来源数限制，停止收集" << std::endl;
                return sources;
            }
        }
    }

    std::cout << "  -> 已收集消息: " << sources.size() << " 条" << std::endl;

    // 2. 从多模态记忆构建多模态来源，按类型应用配额
    int remaining = max_sources > 0 ? (max_sources - static_cast<int>(sources.size())) : 0;
    if (max_sources > 0 && remaining <= 0) {
        std::cout << "  -> 已无剩余配额，停止收集多模态数据" << std::endl;
        return sources;
    }

    // 收集所有多模态数据（不限制数量），然后按类型分组并应用配额
    auto all_multimodal_items = orm.getMultimodalMemoriesByUserId(user_id, 0); // 0 表示获取所有
    
    std::cout << "  -> 获取到多模态数据总数: " << all_multimodal_items.size() << std::endl;

    // 按类型分组
    std::vector<MultimodalMemory> screenshots;
    std::vector<MultimodalMemory> audios;
    std::vector<MultimodalMemory> images;
    std::vector<MultimodalMemory> others;

    for (const auto& item : all_multimodal_items) {
        // Session过滤：如果启用且有有效的session_id，则进行过滤
        // TODO: 需要定义“当前会话”的逻辑，这里简化处理
        
        if (item.data_type == "screenshot") {
            screenshots.push_back(item);
        } else if (item.data_type == "audio") {
            audios.push_back(item);
        } else if (item.data_type == "image" || item.data_type == "camera") {
            images.push_back(item);
        } else {
            others.push_back(item);
        }
    }

    std::cout << "  -> 截图: " << screenshots.size() << ", 音频: " << audios.size() 
              << ", 图像: " << images.size() << ", 其他: " << others.size() << std::endl;

    // 按配额收集截图（最近的N条）
    int screenshot_count = 0;
    int screenshot_limit = config_.max_screenshots_quota;
    if (screenshot_limit > 0 && !screenshots.empty()) {
        std::size_t start = screenshots.size() > static_cast<std::size_t>(screenshot_limit) 
                           ? screenshots.size() - static_cast<std::size_t>(screenshot_limit) 
                           : 0;
        for (std::size_t i = start; i < screenshots.size(); ++i) {
            if (max_sources > 0 && static_cast<int>(sources.size()) >= max_sources) break;
            const auto& item = screenshots[i];
            MemorySource src;
            src.source_category = MemorySource::SourceCategory::Multimodal;
            src.source_id = item.id;
            src.user_id = item.user_id;
            src.agent_id = 0;
            src.session_id = item.session_id;
            src.modality = item.data_type;
            src.text = item.description;
            src.file_path = item.file_path;
            src.metadata = item.metadata;
            src.created_at = item.created_at;
            sources.push_back(std::move(src));
            screenshot_count++;
        }
    }
    std::cout << "  -> 已收集截图: " << screenshot_count << "/" << screenshot_limit << std::endl;

    // 按配额收集音频（最近的N条）
    int audio_count = 0;
    int audio_limit = config_.max_audios_quota;
    if (audio_limit > 0 && !audios.empty()) {
        std::size_t start = audios.size() > static_cast<std::size_t>(audio_limit) 
                           ? audios.size() - static_cast<std::size_t>(audio_limit) 
                           : 0;
        for (std::size_t i = start; i < audios.size(); ++i) {
            if (max_sources > 0 && static_cast<int>(sources.size()) >= max_sources) break;
            const auto& item = audios[i];
            MemorySource src;
            src.source_category = MemorySource::SourceCategory::Multimodal;
            src.source_id = item.id;
            src.user_id = item.user_id;
            src.agent_id = 0;
            src.session_id = item.session_id;
            src.modality = item.data_type;
            src.text = item.description;
            src.file_path = item.file_path;
            src.metadata = item.metadata;
            src.created_at = item.created_at;
            sources.push_back(std::move(src));
            audio_count++;
        }
    }
    std::cout << "  -> 已收集音频: " << audio_count << "/" << audio_limit << std::endl;

    // 按配额收集图像（最近的N条）
    int image_count = 0;
    int image_limit = config_.max_images_quota;
    if (image_limit > 0 && !images.empty()) {
        std::size_t start = images.size() > static_cast<std::size_t>(image_limit) 
                           ? images.size() - static_cast<std::size_t>(image_limit) 
                           : 0;
        for (std::size_t i = start; i < images.size(); ++i) {
            if (max_sources > 0 && static_cast<int>(sources.size()) >= max_sources) break;
            const auto& item = images[i];
            MemorySource src;
            src.source_category = MemorySource::SourceCategory::Multimodal;
            src.source_id = item.id;
            src.user_id = item.user_id;
            src.agent_id = 0;
            src.session_id = item.session_id;
            src.modality = item.data_type;
            src.text = item.description;
            src.file_path = item.file_path;
            src.metadata = item.metadata;
            src.created_at = item.created_at;
            sources.push_back(std::move(src));
            image_count++;
        }
    }
    std::cout << "  -> 已收集图像: " << image_count << "/" << image_limit << std::endl;

    std::cout << "[MemoryGenerationManager::collectSources] 收集完毕，总计: " << sources.size() << " 条来源" << std::endl;

    return sources;
}

void MemoryGenerationManager::saveEpisodicMemory(const EpisodicMemoryGenerator::GeneratedEpisodicMemory& memory, 
                                               int user_id, int agent_id) {
    // 创建临时消息对象以满足接口要求
    MessageManager::Message tempMessage;
    tempMessage.user_id = user_id;
    tempMessage.agent_id = agent_id;
    tempMessage.session_id = 0; // 默认session_id
    
    // 使用EpisodicMemoryGenerator处理和保存
    EpisodicMemoryGenerator generator;
    generator.processAndSave({tempMessage}, user_id, agent_id, tempMessage.session_id, *orchestrator_);
}

void MemoryGenerationManager::saveSemanticMemory(const SemanticMemoryGenerator::GeneratedSemanticMemory& memory, 
                                               int user_id, int agent_id) {
    // 创建临时消息对象以满足接口要求
    MessageManager::Message tempMessage;
    tempMessage.user_id = user_id;
    tempMessage.agent_id = agent_id;
    tempMessage.session_id = 0; // 默认session_id
    
    // 使用SemanticMemoryGenerator处理和保存
    SemanticMemoryGenerator generator;
    generator.processAndSave({tempMessage}, user_id, agent_id, tempMessage.session_id, *orchestrator_);
}

void MemoryGenerationManager::saveCoreMemory(const CoreMemoryGenerator::GeneratedCoreMemory& memory, 
                                           int user_id, int agent_id) {
    // 创建临时消息对象以满足接口要求
    MessageManager::Message tempMessage;
    tempMessage.user_id = user_id;
    tempMessage.agent_id = agent_id;
    tempMessage.session_id = 0; // 默认session_id
    
    // 使用CoreMemoryGenerator处理和保存
    CoreMemoryGenerator generator;
    generator.processAndSave({tempMessage}, user_id, agent_id, tempMessage.session_id, *orchestrator_);
}

void MemoryGenerationManager::saveProceduralMemory(const ProceduralMemoryGenerator::GeneratedProceduralMemory& memory, 
                                                 int user_id, int agent_id) {
    // 创建临时消息对象以满足接口要求
    MessageManager::Message tempMessage;
    tempMessage.user_id = user_id;
    tempMessage.agent_id = agent_id;
    tempMessage.session_id = 0; // 默认session_id
    
    // 使用ProceduralMemoryGenerator处理和保存
    ProceduralMemoryGenerator generator;
    generator.processAndSave({tempMessage}, user_id, agent_id, tempMessage.session_id, *orchestrator_);
}

void MemoryGenerationManager::saveSecretVault(const SecretVaultGenerator::GeneratedSecretVault& memory, 
                                             int user_id, int agent_id) {
    // 创建临时消息对象以满足接口要求
    MessageManager::Message tempMessage;
    tempMessage.user_id = user_id;
    tempMessage.agent_id = agent_id;
    tempMessage.session_id = 0; // 默认session_id
    
    // 使用SecretVaultGenerator处理和保存
    SecretVaultGenerator generator;
    generator.processAndSave({tempMessage}, user_id, agent_id, tempMessage.session_id, *orchestrator_);
}