#pragma once

#include <string>  // Bu satırı ekleyin
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace control {

class ConfigManager {
private:
    nlohmann::json config_;
    std::string config_path_;
    std::unordered_map<std::string, std::string> cache_;
    
public:
    ConfigManager();
    explicit ConfigManager(const std::string& config_path);
    
    bool load(const std::string& path);
    bool reload();
    bool save(const std::string& path = "");
    
    // Config accessors
    template<typename T>
    T get(const std::string& key, const T& default_value = T()) const;
    
    template<typename T>
    void set(const std::string& key, const T& value);
    
    bool has(const std::string& key) const;
    std::vector<std::string> get_keys() const;
    
    // Specific config getters
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    double get_double(const std::string& key, double default_value = 0.0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    const std::string& get_config_path() const { return config_path_; }
    bool is_loaded() const { return !config_.is_null(); }
};

} // namespace control