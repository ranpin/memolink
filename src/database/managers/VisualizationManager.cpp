#include <vector>
#include <string>
#include "VisualizationManager.h"
#include <iostream>
#include <sstream>
#include <sqlite3.h>

VisualizationManager::VisualizationManager(StorageManager& storage) 
    : storage(storage), recallMemoryManager(storage) {
}

std::vector<MemoryNode> VisualizationManager::getMemoryNodes(int userId, int limit) {
    std::vector<MemoryNode> nodes;
    
    // 从回忆记忆中获取节点
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, memory_type, content, created_at FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return nodes;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MemoryNode node;
        node.id = sqlite3_column_int(stmt, 0);
        node.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        node.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        node.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        nodes.push_back(node);
    }
    
    sqlite3_finalize(stmt);
    return nodes;
}

std::vector<MemoryEdge> VisualizationManager::getMemoryEdges(int userId, int limit) {
    std::vector<MemoryEdge> edges;
    
    // 简单示例：创建相邻记忆之间的边
    sqlite3_stmt* stmt;
    std::string query = "SELECT id FROM recall_memory WHERE user_id = ? ORDER BY created_at DESC LIMIT ?;";
    
    if (!storage.executeQuery(query, stmt)) {
        return edges;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);
    
    int prevId = -1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int currentId = sqlite3_column_int(stmt, 0);
        if (prevId != -1) {
            MemoryEdge edge;
            edge.sourceId = prevId;
            edge.targetId = currentId;
            edge.relationship = "temporal";
            edge.weight = 1.0;
            edges.push_back(edge);
        }
        prevId = currentId;
    }
    
    sqlite3_finalize(stmt);
    return edges;
}

std::string VisualizationManager::generateGraphJSON(int userId, int limit) {
    auto nodes = getMemoryNodes(userId, limit);
    auto edges = getMemoryEdges(userId, limit);
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"nodes\": [\n";
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        json << "    {\n";
        json << "      \"id\": " << nodes[i].id << ",\n";
        json << "      \"type\": \"" << nodes[i].type << "\",\n";
        json << "      \"content\": \"" << nodes[i].content << "\",\n";
        json << "      \"timestamp\": \"" << nodes[i].timestamp << "\"\n";
        json << "    }" << (i == nodes.size() - 1 ? "" : ",") << "\n";
    }
    
    json << "  ],\n";
    json << "  \"edges\": [\n";
    
    for (size_t i = 0; i < edges.size(); ++i) {
        json << "    {\n";
        json << "      \"source\": " << edges[i].sourceId << ",\n";
        json << "      \"target\": " << edges[i].targetId << ",\n";
        json << "      \"relationship\": \"" << edges[i].relationship << "\",\n";
        json << "      \"weight\": " << edges[i].weight << "\n";
        json << "    }" << (i == edges.size() - 1 ? "" : ",") << "\n";
    }
    
    json << "  ]\n";
    json << "}";
    
    return json.str();
}

std::string VisualizationManager::generateTimelineData(int userId, int limit) {
    auto messages = recallMemoryManager.list_recall_memories(userId, limit);
    
    std::ostringstream json;
    json << "[\n";
    
    // 限制消息数量
    size_t count = 0;
    for (const auto& message : messages) {
        if (limit > 0 && count >= static_cast<size_t>(limit)) {
            break;
        }
        
        if (count > 0) {
            json << ",\n";
        }
        
        json << "  {\n";
        json << "    \"id\": " << message.id << ",\n";
        json << "    \"type\": \"" << message.message_type << "\",\n";
        json << "    \"content\": \"" << message.content << "\",\n";
        json << "    \"timestamp\": \"" << message.created_at << "\"\n";
        json << "  }";
        
        count++;
    }
    
    json << "\n]";
    return json.str();
}