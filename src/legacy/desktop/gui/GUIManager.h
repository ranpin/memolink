#pragma once

// GUIManager - 图形用户界面管理器

#include <string>
#include <vector>

// 前置声明
class StorageManager;
class RecallMemoryManager;
class ArchivalMemoryManager;
class ModelManager;

struct GUIMemoryItem {
    int id;
    std::string content;
    std::string memory_type;
    std::string created_at;
    std::string agent_id;
};

class GUIManager {
public:
    GUIManager(StorageManager& storage);
    ~GUIManager();
    
    // 初始化GUI
    bool initialize();
    
    // 显示主界面
    void show_main_window();
    
    // 显示记忆列表
    std::vector<GUIMemoryItem> get_memory_list(const std::string& user_id, 
                                              const std::string& filter = "");
    
    // 显示记忆详情
    GUIMemoryItem get_memory_details(int memory_id);
    
    // 搜索记忆
    std::vector<GUIMemoryItem> search_memories(const std::string& user_id,
                                              const std::string& query);
    
    // 添加新记忆
    bool add_memory(const std::string& user_id,
                   const std::string& content,
                   const std::string& memory_type,
                   const std::string& metadata = "");
    
    // 编辑记忆
    bool edit_memory(int memory_id,
                    const std::string& content,
                    const std::string& metadata = "");
    
    // 删除记忆
    bool delete_memory(int memory_id);
    
    // 显示统计信息
    std::string get_statistics(const std::string& user_id);
    
    // 显示设置界面
    void show_settings();
    
    // 更新设置
    bool update_settings(const std::string& key, const std::string& value);

private:
    StorageManager& storage_;
    
    // GUI状态
    bool initialized_;
    std::string current_user_id_;
    
    // 私有辅助方法
    void render_main_window();
    void render_memory_list(const std::vector<GUIMemoryItem>& memories);
    void render_memory_details(const GUIMemoryItem& memory);
    void render_search_results(const std::vector<GUIMemoryItem>& results);
    void render_statistics(const std::string& stats);
    void render_settings();
    
    // 模拟GUI渲染函数
    void draw_window(const std::string& title);
    void draw_button(const std::string& label);
    void draw_text(const std::string& text);
    void draw_input_field(const std::string& label, std::string& value);
};