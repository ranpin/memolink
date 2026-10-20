#ifndef MEMORY_ORCHESTRATOR_H
#define MEMORY_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../core/StorageManager.h"
#include "../../models/ModelManager.h"
#include "../../models/TextEmbeddingGenerator.h"
#include "RecallMemoryManager.h"
#include "ArchivalMemoryManager.h"
#include "../../models/EmbeddingIndexManager.h"

// 一条检索命中。
struct MemoryHit {
    int id = 0;
    std::string type;       // 记忆类型（存进 recall_memory.message_type）
    std::string content;
    float score = 0.f;      // 与查询向量的余弦相似度
    bool sensitive = false; // 是否敏感（从 metadata 的 sensitive 字段解析）
    std::string timestamp;
};

// 端侧记忆编排器。
//
// 设计要点（P3）：
//  - 只有 recall_memory / resource_memory 两张表带 embedding 列，六个分表 manager
//    （semantic/episodic/procedural/core/...）都没有 embedding 支持。因此用
//    recall_memory 作为「统一的带 embedding 存储」：记忆类型写进 message_type、
//    敏感标记写进 metadata，从而复用 RecallMemoryManager 现成的
//    add_recall_memory / saveEmbedding / searchByEmbedding，不改动那 2618 行。
//  - embedding 由 TextEmbeddingGenerator 产出（config.ini [edge] 配上路径即走本地
//    ONNX，见 P2 接线）；写入时算一次并存进 embedding 列。
//  - 分类型有界淘汰：每类一个容量上限，超限按 id（=写入顺序，FIFO）淘汰最旧。
class MemoryOrchestrator {
private:
    StorageManager& storage;
    ModelManager& model;
    RecallMemoryManager recallMemoryManager;
    ArchivalMemoryManager archivalMemoryManager;
    EmbeddingIndexManager embeddingIndexManager;

    std::unique_ptr<TextEmbeddingGenerator> embedder_;  // 本地 ONNX embedding（P2 接线）
    std::map<std::string, int> type_capacity_;          // 分类型容量上限
    int default_capacity_ = 1000;                       // 默认容量（大 = 默认不淘汰）
    int user_id_ = 1;
    int agent_id_ = 1;    // recall_memory 对 user/agent/session 都有外键，
    int session_id_ = 1;  // 且 StorageManager::initialize() 开了 foreign_keys=ON，
                          // 故用一组默认实体满足约束（见 ensureDefaultEntities）

    bool parseSensitive(const std::string& metadata) const;
    int evictType(const std::string& type);
    void ensureDefaultEntities();

public:
    MemoryOrchestrator(StorageManager& storage, ModelManager& model);

    // ---- 原有四个方法（填充实空壳；保留签名以兼容既有调用）----
    bool processMemory(const std::string& userId, const std::string& sessionId,
                       const std::string& agentId, const std::string& memoryType,
                       const std::string& content, const std::string& metadata);

    std::string retrieveMemory(const std::string& userId, const std::string& query,
                               const std::string& queryType, int topK = 10);

    bool add_memory_with_embedding(const std::string& memoryType,
                                   const std::string& content,
                                   const std::vector<float>& embedding,
                                   const std::string& metadata);

    std::vector<int> search_memory_by_embedding(const std::vector<float>& queryEmbedding,
                                                int topK);

    // ---- 新增：端侧记忆系统主 API ----
    // 写入一条记忆：编码 → 落库（message_type=type）→ 存 embedding → 按类型淘汰。
    bool write(const std::string& type, const std::string& content,
               bool sensitive = false, const std::string& metadata = "{}");

    // 跨类型检索：编码查询 → searchByEmbedding → 重算余弦 → 返回 topK 命中。
    std::vector<MemoryHit> retrieve(const std::string& query, int topK = 4);

    // 对全部已见类型执行有界淘汰，返回总淘汰条数。
    int evict();

    // 从 JSON 语料加载种子记忆（格式见 bench/corpus/*.json），返回成功写入条数。
    int loadCorpus(const std::string& corpusPath);

    void setTypeCapacity(const std::string& type, int cap);
    void setDefaultCapacity(int cap) { default_capacity_ = cap; }
    void setUserId(int uid) { user_id_ = uid; }

    // 计数（验证淘汰是否真的生效用）
    int countByType(const std::string& type);
    int countAll();
};

#endif // MEMORY_ORCHESTRATOR_H
