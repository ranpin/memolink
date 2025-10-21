#pragma once

// AgentManager - 代理管理器，负责创建和管理所有代理实例

#include "../database/core/StorageManager.h"
#include "../models/ModelManager.h"
#include "AgentWrapper.h"
#include <memory>
#include <map>
#include <string>

class AgentManager {
private:
    StorageManager& storage_;
    ModelManager& model_;
    
    // 代理包装器实例
    std::unique_ptr<AgentWrapper> agent_wrapper_;

public:
    // 构造函数
    AgentManager(StorageManager& storage, ModelManager& model);
    
    // 代理管理接口
    AgentWrapper* get_agent_wrapper();
    
    // 会话管理
    int create_session(int user_id, const std::string& session_name = "");
    bool end_session(int session_id);
    std::string get_session_info(int session_id);

    // 代理管理
    int create_agent(const std::string& name, const std::string& type, const std::string& status);
    int get_agent_id_by_name(const std::string& name);
};