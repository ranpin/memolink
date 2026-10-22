#include <string>
#include <memory>
#include "legacy/agents/AgentManager.h"
#include "legacy/core-v1/UserManager.h"
#include "legacy/core-v1/SessionManager.h"
#include "infra/storage/TimeUtils.h"
#include <iostream>

AgentManager::AgentManager(StorageManager& storage, ModelManager& model) 
    : storage_(storage), model_(model) {
    // 初始化代理包装器
    agent_wrapper_ = std::make_unique<AgentWrapper>(storage, model);
}

AgentWrapper* AgentManager::get_agent_wrapper() {
    return agent_wrapper_.get();
}

int AgentManager::create_session(int user_id, const std::string& session_name) {
    SessionManager session_manager(storage_);
    SessionItem session_item;
    session_item.user_id = std::to_string(user_id);
    
    // 获取当前北京时间作为开始时间
    session_item.start_time = TimeUtils::getCurrentTimestamp();
    
    session_item.end_time = "";
    session_item.status = "active";
    session_item.metadata = "{}";
    
    if (session_manager.create_session(session_item)) {
        std::cout << "Created session for user " << user_id << std::endl;
        // 返回新创建的会话ID（这里简化处理，实际应该从session_manager获取）
        return 1;
    }
    return -1;
}

bool AgentManager::end_session(int session_id) {
    SessionManager session_manager(storage_);
    return session_manager.end_session(session_id);
}

std::string AgentManager::get_session_info(int session_id) {
    SessionManager session_manager(storage_);
    // 注意：此处需要提供用户ID参数
    auto sessions = session_manager.list_sessions("default_user");
    
    for (const auto& session : sessions) {
        if (session.id == session_id) {
            std::string info = "Session ID: " + std::to_string(session_id) + "\n";
            info += "User ID: " + session.user_id + "\n";
            info += "Start Time: " + session.start_time + "\n";
            info += "End Time: " + session.end_time + "\n";
            info += "Status: " + session.status + "\n";
            return info;
        }
    }
    
    return "Session not found";
}

// 添加创建代理的方法
int AgentManager::create_agent(const std::string& name, const std::string& type, const std::string& status) {
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO agent (name, type, status) VALUES (?, ?, ?);";
    
    if (!storage_.executeQuery(query, stmt)) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return -1;
    }
    
    // 获取新创建的代理ID
    query = "SELECT last_insert_rowid();";
    if (!storage_.executeQuery(query, stmt)) {
        return -1;
    }
    
    int agent_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        agent_id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return agent_id;
}

// 添加获取代理ID的方法
int AgentManager::get_agent_id_by_name(const std::string& name) {
    sqlite3_stmt* stmt;
    std::string query = "SELECT id FROM agent WHERE name = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    
    int agent_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        agent_id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return agent_id;
}