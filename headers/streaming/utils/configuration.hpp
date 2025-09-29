// include/streaming/utils/configuration.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace streaming {

class Configuration {
public:
    using Value = std::variant<int, double, bool, std::string, std::vector<std::string>>;
    
    Configuration();
    ~Configuration() = default;
    
    // Load configuration from files
    bool load_from_file(const std::string& filename);
    bool load_from_string(const std::string& yaml_content);
    bool load_from_environment();
    
    // Configuration access
    template<typename T>
    T get(const std::string& key, const T& default_value = T()) const;
    
    template<typename T>
    void set(const std::string& key, const T& value);
    
    // Validation
    bool validate() const;
    std::vector<std::string> get_validation_errors() const;
    
    // Watch for changes (hot reload)
    void enable_hot_reload(bool enable);
    void set_change_callback(std::function<void(const std::string& key)> callback);

    // Configuration sections
    class Section {
    public:
        Section(const std::string& name, const Configuration* parent);
        
        template<typename T>
        T get(const std::string& key, const T& default_value = T()) const;
        
    private:
        std::string name_;
        const Configuration* parent_;
    };
    
    Section get_section(const std::string& section_name) const;

private:
    bool parse_yaml_node(const YAML::Node& node, const std::string& prefix = "");
    Value convert_yaml_value(const YAML::Node& node) const;
    
    std::unordered_map<std::string, Value> config_map_;
    mutable std::mutex config_mutex_;
    std::string config_file_;
    std::atomic<bool> hot_reload_enabled_{false};
    std::function<void(const std::string&)> change_callback_;
};

// Global configuration instance
Configuration& get_global_config();

} // namespace streaming

// Template implementation
template<typename T>
T streaming::Configuration::get(const std::string& key, const T& default_value) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = config_map_.find(key);
    if (it == config_map_.end()) {
        return default_value;
    }
    
    try {
        return std::get<T>(it->second);
    } catch (const std::bad_variant_access&) {
        return default_value;
    }
}

template<typename T>
void streaming::Configuration::set(const std::string& key, const T& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_map_[key] = value;
    
    if (change_callback_) {
        change_callback_(key);
    }
}