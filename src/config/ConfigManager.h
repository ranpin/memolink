#pragma once
#include <string>
#include <map>

class ConfigManager {
public:
    ConfigManager(const std::string& configPath);
    bool load();
    std::string get(const std::string& key) const;

private:
    std::string configPath_;
    std::map<std::string, std::string> config_;
};
