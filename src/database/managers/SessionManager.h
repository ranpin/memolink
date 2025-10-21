#pragma once
#include "../core/StorageManager.h"
#include <string>
#include <vector>

struct SessionItem {
    int id;
    std::string user_id;
    std::string token;
    std::string start_time;
    std::string end_time;
    std::string status;
    std::string device_info;
    std::string metadata;
};

class SessionManager {
public:
    SessionManager(StorageManager& storage);
    
    // 会话操作
    bool create_session(const SessionItem& item);
    std::vector<SessionItem> list_sessions(const std::string& user_id);
    bool update_session(int id, const SessionItem& item);
    bool delete_session(int id);
    
    // 获取活跃会话
    SessionItem get_active_session(const std::string& user_id);
    
    // 结束会话
    bool end_session(int id);

private:
    StorageManager& storage_;
};