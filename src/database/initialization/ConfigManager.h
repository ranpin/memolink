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
    
    // 记忆生成配置
    int getMemoryGenerationMessageThreshold() const;
    int getMemoryGenerationTimeIntervalMinutes() const;
    float getMemoryGenerationConfidenceThreshold() const;
    int getMemoryGenerationMaxSourcesPerGeneration() const;
    bool getMemoryGenerationEnableProvenance() const;
    bool getMemoryGenerationEnableEpisodic() const;
    bool getMemoryGenerationEnableSemantic() const;
    bool getMemoryGenerationEnableCore() const;
    bool getMemoryGenerationEnableProcedural() const;
    bool getMemoryGenerationEnableSecretVault() const;
    
    // 溯源范围细化配置
    int getMemoryGenerationMaxMessagesQuota() const;
    int getMemoryGenerationMaxScreenshotsQuota() const;
    int getMemoryGenerationMaxAudiosQuota() const;
    int getMemoryGenerationMaxImagesQuota() const;
    bool getMemoryGenerationFilterByCurrentSession() const;
    
    // 记忆调度配置
    bool getMemorySchedulerEnabled() const;
    int getMemorySchedulerIntervalSeconds() const;
};

#endif // CONFIG_MANAGER_H