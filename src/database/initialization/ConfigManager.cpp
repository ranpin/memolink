#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <sstream>

// 静态成员初始化
ConfigManager* ConfigManager::instance = nullptr;

ConfigManager::ConfigManager(const std::string& configFile) : configFile(configFile) {
    // 尝试多个可能的配置文件路径
    std::vector<std::string> configPaths = {
        configFile,                    // 默认路径
        "../" + configFile,           // 上级目录
        "../../" + configFile,        // 上上级目录
        "config/" + configFile        // config子目录
    };
    
    std::string actualConfigFile = configFile;
    for (const auto& path : configPaths) {
        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
            actualConfigFile = path;
            break;
        }
    }
    
    // 重新赋值 configFile 成员变量
    // 注意：由于 configFile 是 const 成员变量，我们不能直接修改它
    // 所以我们使用 const_cast 来修改它，但这只在构造函数中是安全的
    const_cast<std::string&>(this->configFile) = actualConfigFile;
    
    loadConfig();
}

ConfigManager& ConfigManager::getInstance() {
    if (instance == nullptr) {
        instance = new ConfigManager("config.ini");
    }
    return *instance;
}

void ConfigManager::destroyInstance() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

bool ConfigManager::loadConfig() {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << configFile << std::endl;
        return false;
    }

    std::string currentSection;
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 处理节头 [section]
        if (line[0] == '[') {
            size_t endBracket = line.find(']');
            if (endBracket != std::string::npos) {
                currentSection = line.substr(1, endBracket - 1);
                continue;
            }
        }
        
        // 解析键值对
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            
            // 去除前后空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 移除可能存在的引号
            if (!value.empty() && ((value.front() == '"' && value.back() == '"') || 
                                   (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }
            
            // 构造完整的键名（包含节名）
            std::string fullKey = key;
            if (!currentSection.empty()) {
                fullKey = currentSection + "." + key;
            }
            
            config[fullKey] = value;
        }
    }
    
    file.close();
    return true;
}

bool ConfigManager::saveConfig() {
    std::ofstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "无法保存配置文件: " << configFile << std::endl;
        return false;
    }
    
    for (const auto& pair : config) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    file.close();
    return true;
}

void ConfigManager::setConfig(const std::string& key, const std::string& value) {
    config[key] = value;
    saveConfig();
}

std::string ConfigManager::getConfig(const std::string& key, const std::string& defaultValue) const {
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second;
    }
    return defaultValue;
}

// API配置
std::string ConfigManager::getApiKey() const {
    return getConfig("api.key", "");
}

void ConfigManager::setApiKey(const std::string& apiKey) {
    setConfig("api.key", apiKey);
}

// 本地API配置
std::string ConfigManager::getLocalApiUrl() const {
    return getConfig("api.local_url", "");
}

void ConfigManager::setLocalApiUrl(const std::string& localApiUrl) {
    setConfig("api.local_url", localApiUrl);
}

// 数据库配置
std::string ConfigManager::getDatabasePath() const {
    return getConfig("database.db_path", "mirix.db");
}

void ConfigManager::setDatabasePath(const std::string& dbPath) {
    setConfig("database.db_path", dbPath);
}

// 文本模型配置
std::string ConfigManager::getTextModelName() const {
    return getConfig("model.text_model_name", "qwen-plus");
}

void ConfigManager::setTextModelName(const std::string& modelName) {
    setConfig("model.text_model_name", modelName);
}

// 嵌入模型配置
std::string ConfigManager::getEmbeddingModelName() const {
    return getConfig("model.embedding_model_name", "text-embedding-v1");
}

void ConfigManager::setEmbeddingModelName(const std::string& modelName) {
    setConfig("model.embedding_model_name", modelName);
}

// 多模态模型配置
std::string ConfigManager::getMultimodalModelName() const {
    return getConfig("model.multimodal_model_name", "qwen-vl-plus");
}

void ConfigManager::setMultimodalModelName(const std::string& modelName) {
    setConfig("model.multimodal_model_name", modelName);
}

// 记忆生成配置
int ConfigManager::getMemoryGenerationMessageThreshold() const {
    std::string value = getConfig("memory_generation.message_threshold", "5");
    try {
        return std::stoi(value);
    } catch (...) {
        return 5;
    }
}

int ConfigManager::getMemoryGenerationTimeIntervalMinutes() const {
    std::string value = getConfig("memory_generation.time_interval_minutes", "30");
    try {
        return std::stoi(value);
    } catch (...) {
        return 30;
    }
}

float ConfigManager::getMemoryGenerationConfidenceThreshold() const {
    std::string value = getConfig("memory_generation.confidence_threshold", "0.5");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.5f;
    }
}

int ConfigManager::getMemoryGenerationMaxSourcesPerGeneration() const {
    std::string value = getConfig("memory_generation.max_sources_per_generation", "50");
    try {
        return std::stoi(value);
    } catch (...) {
        return 50;
    }
}

bool ConfigManager::getMemoryGenerationEnableProvenance() const {
    std::string value = getConfig("memory_generation.enable_provenance", "true");
    return (value == "true" || value == "1" || value == "yes");
}

bool ConfigManager::getMemoryGenerationEnableEpisodic() const {
    std::string value = getConfig("memory_generation.enable_episodic", "true");
    return (value == "true" || value == "1" || value == "yes");
}

bool ConfigManager::getMemoryGenerationEnableSemantic() const {
    std::string value = getConfig("memory_generation.enable_semantic", "true");
    return (value == "true" || value == "1" || value == "yes");
}

bool ConfigManager::getMemoryGenerationEnableCore() const {
    std::string value = getConfig("memory_generation.enable_core", "true");
    return (value == "true" || value == "1" || value == "yes");
}

bool ConfigManager::getMemoryGenerationEnableProcedural() const {
    std::string value = getConfig("memory_generation.enable_procedural", "true");
    return (value == "true" || value == "1" || value == "yes");
}

bool ConfigManager::getMemoryGenerationEnableSecretVault() const {
    std::string value = getConfig("memory_generation.enable_secret_vault", "true");
    return (value == "true" || value == "1" || value == "yes");
}

// 溯源范围细化配置
int ConfigManager::getMemoryGenerationMaxMessagesQuota() const {
    std::string value = getConfig("memory_generation.max_messages_quota", "10");
    try {
        return std::stoi(value);
    } catch (...) {
        return 10;
    }
}

int ConfigManager::getMemoryGenerationMaxScreenshotsQuota() const {
    std::string value = getConfig("memory_generation.max_screenshots_quota", "5");
    try {
        return std::stoi(value);
    } catch (...) {
        return 5;
    }
}

int ConfigManager::getMemoryGenerationMaxAudiosQuota() const {
    std::string value = getConfig("memory_generation.max_audios_quota", "3");
    try {
        return std::stoi(value);
    } catch (...) {
        return 3;
    }
}

int ConfigManager::getMemoryGenerationMaxImagesQuota() const {
    std::string value = getConfig("memory_generation.max_images_quota", "5");
    try {
        return std::stoi(value);
    } catch (...) {
        return 5;
    }
}

bool ConfigManager::getMemoryGenerationFilterByCurrentSession() const {
    std::string value = getConfig("memory_generation.filter_by_current_session", "false");
    return (value == "true" || value == "1" || value == "yes");
}

// 记忆调度配置
bool ConfigManager::getMemorySchedulerEnabled() const {
    std::string value = getConfig("memory_scheduler.enabled", "true");
    return (value == "true" || value == "1" || value == "yes");
}

int ConfigManager::getMemorySchedulerIntervalSeconds() const {
    std::string value = getConfig("memory_scheduler.interval_seconds", "300");
    try {
        return std::stoi(value);
    } catch (...) {
        return 300;
    }
}