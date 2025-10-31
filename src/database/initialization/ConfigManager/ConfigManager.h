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
};

#endif // CONFIG_MANAGER_H