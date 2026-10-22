#include "memory/api/MemorySystem.h"

// 成员按 store_ → resolver_ → extractor_ 顺序初始化：resolver_ 持有 store_ 的引用，
// 必须保证 store_ 先构造完成。
MemorySystem::MemorySystem(StorageManager& storage, QwenAPI* llm)
    : store_(storage), resolver_(store_, llm), extractor_(llm) {}

std::vector<CandidateFact> MemorySystem::extract(const std::string& userTurn,
                                                 const std::string& assistantTurn) {
    return extractor_.extract(userTurn, assistantTurn);
}

IngestResult MemorySystem::ingest(const std::string& text, MemoryType type,
                                  const std::string& owner, const std::string& scope,
                                  const std::string& authority, const std::string& sourceSession,
                                  int sourceTurn) {
    return resolver_.ingest(text, type, owner, scope, authority, sourceSession, sourceTurn);
}

std::vector<Memory> MemorySystem::recall(const std::string& query, int topK,
                                         std::optional<MemoryType> filter, bool includeHistory) {
    return store_.recall(query, topK, filter, includeHistory);
}

std::vector<Memory> MemorySystem::recallTyped(const std::string& query, int topK,
                                              bool includeHistory) {
    return store_.recallTyped(query, topK, includeHistory);
}

bool MemorySystem::update(int id, const std::string& newContent) {
    return store_.update(id, newContent);
}

bool MemorySystem::remove(int id) { return store_.remove(id); }

std::vector<Memory> MemorySystem::list(std::optional<MemoryType> type, int limit,
                                       bool includeInvalid) {
    return store_.list(type, limit, includeInvalid);
}

std::vector<Memory> MemorySystem::history(int id) { return store_.history(id); }

Memory MemorySystem::get(int id) { return store_.get(id); }

MemoryStats MemorySystem::stats() { return store_.stats(); }
