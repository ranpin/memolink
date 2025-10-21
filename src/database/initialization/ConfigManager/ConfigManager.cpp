#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <sstream>

// 静态成员初始化
ConfigManager* ConfigManager::instance = nullptr;

ConfigManager::ConfigManager(const std::string& configFile) : configFile(configFile) {
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