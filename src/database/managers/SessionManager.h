#pragma once
#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <optional>
#include <vector>
#include "../core/StorageManager.h"
#include "../models/MemoryModels.h"
#include <vector>
#include <optional>

class SessionManager {
private:
    StorageManager& storage_;

public:
    explicit SessionManager(StorageManager& storage) : storage_(storage) {}

    /**
     * @brief 创建一个新的会话
     * @param user_id 用户ID
     * @param metadata 会话元数据
     * @return 会话ID，失败返回-1
     */
    int create_session(int user_id, const std::string& metadata);

    /**
     * @brief 结束会话
     * @param session_id 会话ID
     * @return 是否成功
     */
    bool end_session(int session_id);

    /**
     * @brief 获取会话信息
     * @param session_id 会话ID
     * @return 会话信息，不存在则返回空
     */
    std::optional<Session> get_session(int session_id);

    /**
     * @brief 获取用户的所有会话
     * @param user_id 用户ID
     * @param limit 限制数量
     * @return 会话列表
     */
    std::vector<Session> get_user_sessions(int user_id, int limit = 10);

    /**
     * @brief 更新会话元数据
     * @param session_id 会话ID
     * @param metadata 新的元数据
     * @return 是否成功
     */
    bool update_session_metadata(int session_id, const std::string& metadata);
};

#endif // SESSION_MANAGER_H