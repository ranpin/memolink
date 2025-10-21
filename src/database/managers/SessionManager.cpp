#include "SessionManager.h"
#include "../../utils/TimeUtils.h"
#include <sqlite3.h>
#include <iostream>
#include <sstream>

SessionManager::SessionManager(StorageManager& storage) : storage_(storage) {}

bool SessionManager::create_session(const SessionItem& item) {
    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO session (user_id, start_time, end_time, status, metadata) VALUES (?, ?, ?, ?, ?);";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, std::stoi(item.user_id));
    sqlite3_bind_text(stmt, 2, item.start_time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, item.end_time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, item.status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.metadata.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<SessionItem> SessionManager::list_sessions(const std::string& user_id) {
    std::vector<SessionItem> sessions;
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, start_time, end_time, status, metadata FROM session WHERE user_id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return sessions;
    }
    
    sqlite3_bind_int(stmt, 1, std::stoi(user_id));
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = std::to_string(sqlite3_column_int(stmt, 1));
        item.start_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        item.end_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        sessions.push_back(item);
    }
    
    sqlite3_finalize(stmt);
    return sessions;
}

bool SessionManager::update_session(int id, const SessionItem& item) {
    sqlite3_stmt* stmt;
    std::string query = "UPDATE session SET user_id = ?, start_time = ?, end_time = ?, status = ?, metadata = ? WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, std::stoi(item.user_id));
    sqlite3_bind_text(stmt, 2, item.start_time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, item.end_time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, item.status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, item.metadata.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

bool SessionManager::delete_session(int id) {
    sqlite3_stmt* stmt;
    std::string query = "DELETE FROM session WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}

SessionItem SessionManager::get_active_session(const std::string& user_id) {
    SessionItem item;
    item.id = -1; // 表示未找到
    
    sqlite3_stmt* stmt;
    std::string query = "SELECT id, user_id, start_time, end_time, status, metadata FROM session WHERE user_id = ? AND status = 'active' ORDER BY start_time DESC LIMIT 1;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return item;
    }
    
    sqlite3_bind_int(stmt, 1, std::stoi(user_id));
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        item.id = sqlite3_column_int(stmt, 0);
        item.user_id = std::to_string(sqlite3_column_int(stmt, 1));
        item.start_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        item.end_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        item.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        item.metadata = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    }
    
    sqlite3_finalize(stmt);
    return item;
}

bool SessionManager::end_session(int id) {
    sqlite3_stmt* stmt;
    // 使用TimeUtils获取当前时间戳
    std::string currentTime = TimeUtils::getCurrentTimestamp();
    std::string query = "UPDATE session SET end_time = ?, status = 'ended' WHERE id = ?;";
    
    if (!storage_.executeQuery(query, stmt)) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, currentTime.c_str(), -1, SQLITE_STATIC); // 绑定当前时间
    sqlite3_bind_int(stmt, 2, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(storage_.getDatabase()) << std::endl;
        return false;
    }
    
    return true;
}
