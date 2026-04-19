#include "SessionManager.h"
#include <iostream>
#include <sqlite3.h>

int SessionManager::create_session(int user_id, const std::string& metadata) {
    try {
        // 创建Session对象
        Session session{};
        session.user_id = user_id;
        session.metadata = metadata;
        
        // 插入数据库
        auto& orm = storage_.getORMWrapper();
        int session_id = orm.insert(session);
        
        // 更新开始时间为当前时间戳
        auto session_opt = orm.getById<Session>(session_id);
        if (session_opt.has_value()) {
            session = session_opt.value();
            session.start_time = "CURRENT_TIMESTAMP"; // 这将在数据库中被解释为当前时间
            
            // 保存更新后的会话
            orm.update(session);
        }
        
        return session_id;
    } catch (const std::exception& e) {
        std::cerr << "Error creating session: " << e.what() << std::endl;
        return -1;
    }
}

bool SessionManager::end_session(int session_id) {
    try {
        auto& orm = storage_.getORMWrapper();
        
        // 获取现有会话
        auto session_opt = orm.getById<Session>(session_id);
        if (!session_opt.has_value()) {
            return false;
        }
        
        // 更新结束时间
        auto session = session_opt.value();
        session.end_time = "CURRENT_TIMESTAMP"; // 这将在数据库中被解释为当前时间
        
        // 保存更新后的会话
        orm.update(session);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error ending session: " << e.what() << std::endl;
        return false;
    }
}

std::optional<Session> SessionManager::get_session(int session_id) {
    try {
        auto& orm = storage_.getORMWrapper();
        return orm.getById<Session>(session_id);
    } catch (const std::exception& e) {
        std::cerr << "Error getting session: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Session> SessionManager::get_user_sessions(int user_id, int limit) {
    try {
        auto& orm = storage_.getORMWrapper();
        return orm.getByCondition<Session>(
            sqlite_orm::where(sqlite_orm::c(&Session::user_id) == user_id)
        );
    } catch (const std::exception& e) {
        std::cerr << "Error getting user sessions: " << e.what() << std::endl;
        return {};
    }
}

bool SessionManager::update_session_metadata(int session_id, const std::string& metadata) {
    try {
        auto& orm = storage_.getORMWrapper();
        
        // 获取现有会话
        auto session_opt = orm.getById<Session>(session_id);
        if (!session_opt.has_value()) {
            return false;
        }
        
        // 更新元数据
        auto session = session_opt.value();
        session.metadata = metadata;
        
        // 保存更新后的会话
        orm.update(session);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating session metadata: " << e.what() << std::endl;
        return false;
    }
}