#include "control/config_manager.hpp"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // Assuming nlohmann_json is used for JSON parsing

namespace control {

ConfigManager::ConfigManager() {
    // Constructor logic (if any)
}

bool ConfigManager::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return false;
        }

        nlohmann::json configJson;
        file >> configJson;

        // Parse and store configuration values
        // Example: this->configValue = configJson["key"].get<std::string>();

        std::cout << "Config loaded successfully from: " << path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

} // namespace control