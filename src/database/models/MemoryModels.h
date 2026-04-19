#ifndef MEMORY_MODELS_H
#define MEMORY_MODELS_H

#include <string>
#include <vector>
#include <chrono>

// 用户表
struct User {
    int id = 0;
    std::string name;
    std::string email;
    std::string password_hash = "";
    std::string created_at = "";
};

// 消息记忆表
struct MessageMemory {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    int session_id = 0;
    std::string content;
    std::string created_at = "";
    bool processed = false;
};

// 情景记忆表
struct EpisodicMemory {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    int session_id = 0;
    std::string event_description;
    std::string participants = "";
    std::string location = "";
    std::string timestamp = "";
    int importance = 0;
    std::string metadata = "{}";
    std::string created_at = "";
};

// 语义记忆表
struct SemanticMemory {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    std::string fact;
    std::string category = "";
    double confidence = 0.0;
    std::string metadata = "{}";
    std::string created_at = "";
};

// 程序记忆表
struct ProceduralMemory {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    std::string name;
    std::string description;
    std::string steps;
    std::string metadata = "{}";
    std::string created_at = "";
};



// 私密信息库表
struct SecretVault {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    std::string category = "";
    std::string title;
    std::string content;
    std::string metadata = "{}";
    std::string created_at = "";
};

// 核心记忆表
struct CoreMemory {
    int id = 0;
    int user_id = 0;
    int agent_id = 0;
    std::string memory_type;
    std::string content;
    int priority = 0;
    std::string metadata = "{}";
    std::string created_at = "";
};

// 多模态记忆表
struct MultimodalMemory {
    int id = 0;
    int user_id = 0;
    int session_id = 0;
    std::string data_type;  // image, audio, screenshot
    std::string file_path;
    std::string description = "";  // AI生成的描述
    std::string metadata = "{}";
    std::string created_at = "";
};

// 记忆溯源表
struct MemoryProvenance {
    int id = 0;
    std::string memory_type;   // episodic/semantic/procedural/core/secret_vault 等
    int memory_id = 0;         // 对应具体记忆表的id
    std::string source_type;   // message/multimodal 等
    int source_id = 0;         // 对应源表的id
    std::string relation_type = ""; // 关系类型: evidence/supporting/summary_of 等
    std::string created_at = "";
};

// 会话表
struct Session {
    int id = 0;
    int user_id = 0;
    std::string start_time;
    std::string end_time = "";
    std::string metadata = "{}";
};

#endif // MEMORY_MODELS_H