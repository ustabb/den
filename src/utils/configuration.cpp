// src/utils/configuration.cpp
#include "streaming/utils/configuration.hpp"
#include "streaming/utils/logger.hpp"
#include <fstream>
#include <filesystem>

namespace streaming {

Configuration& get_global_config() {
    static Configuration global_config;
    return global_config;
}

bool Configuration::load_from_file(const std::string& filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            LOG_ERROR("Configuration file not found: {}", filename);
            return false;
        }
        
        YAML::Node config = YAML::LoadFile(filename);
        config_file_ = filename;
        
        return parse_yaml_node(config);
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("YAML parsing error in {}: {}", filename, e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading configuration from {}: {}", filename, e.what());
        return false;
    }
}

bool Configuration::parse_yaml_node(const YAML::Node& node, const std::string& prefix) {
    try {
        for (const auto& pair : node) {
            std::string key = pair.first.as<std::string>();
            std::string full_key = prefix.empty() ? key : prefix + "." + key;
            
            if (pair.second.IsMap()) {
                parse_yaml_node(pair.second, full_key);
            } else {
                config_map_[full_key] = convert_yaml_value(pair.second);
            }
        }
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error parsing YAML node: {}", e.what());
        return false;
    }
}

Configuration::Value Configuration::convert_yaml_value(const YAML::Node& node) const {
    try {
        if (node.IsScalar()) {
            // Try different types
            try {
                return node.as<int>();
            } catch (...) {}
            
            try {
                return node.as<double>();
            } catch (...) {}
            
            try {
                return node.as<bool>();
            } catch (...) {}
            
            return node.as<std::string>();
        } else if (node.IsSequence()) {
            std::vector<std::string> result;
            for (const auto& item : node) {
                result.push_back(item.as<std::string>());
            }
            return result;
        }
        
        return std::string("unknown");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error converting YAML value: {}", e.what());
        return std::string("error");
    }
}

} // namespace streaming