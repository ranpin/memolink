#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>

class ConfigManager {
private:
    std::map<std::string, std::string> config;
    std::string configFile;
    static ConfigManager* instance;

    ConfigManager(const std::string& configFile = "config.ini");
    
public:
    static ConfigManager& getInstance();
    static void destroyInstance();
    
    bool loadConfig();
    bool saveConfig();
    void setConfig(const std::string& key, const std::string& value);
    std::string getConfig(const std::string& key, const std::string& defaultValue = "") const;
    
    // API配置
    std::string getApiKey() const;
    void setApiKey(const std::string& apiKey);
    
    // 本地API配置
    std::string getLocalApiUrl() const;
    void setLocalApiUrl(const std::string& localApiUrl);

    // Q9：VLM（视觉语言模型）本地服务 URL——caption-then-embed 的图像描述端点。
    // 与 local_url（文本 LLM）分开：座舱多模态轮次要同时调两个 llama-server 实例。
    std::string getVlmApiUrl() const;
    void setVlmApiUrl(const std::string& vlmApiUrl);
    
    // 数据库配置
    std::string getDatabasePath() const;
    void setDatabasePath(const std::string& dbPath);
    
    // 模型配置
    std::string getTextModelName() const;
    void setTextModelName(const std::string& modelName);
    
    std::string getEmbeddingModelName() const;
    void setEmbeddingModelName(const std::string& modelName);
    
    std::string getMultimodalModelName() const;
    void setMultimodalModelName(const std::string& modelName);

    // 端侧推理配置（仅 MEMOLINK_ENABLE_EDGE 构建使用）
    // 两个路径都非空时，文本 embedding 走本地 ONNX，不再走云端 DashScope。
    std::string getEdgeEmbeddingModelPath() const;
    std::string getEdgeTokenizerPath() const;
    int getEdgeEmbeddingThreads() const;
};

#endif // CONFIG_MANAGER_H