#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <fmt/format.h>

namespace debugger {
	class Configuration {
	public:
		Configuration() = default;
		~Configuration() = default;

        // Load a JSON file into the config
        void loadFromFile(const std::string& filePath) {
            try {
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Could not open config file: " + filePath);
                }
                nlohmann::json jsonFile;
                file >> jsonFile;
                merge(jsonFile); // Merge into existing config
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading config: " << e.what() << '\n';
            }
        }

        // Overlay another JSON config
        void merge(const nlohmann::json& otherConfig) {
            config_.merge_patch(otherConfig);
        }

        // Access config values
        template <typename T>
        T get(const std::string& key, const T& defaultValue = T()) const {
            try {
                return config_.at(key).get<T>();
            }
            catch (...) {
                return defaultValue;
            }
        }

        // Debug or inspect config
        void print() const {
            std::cout << config_.dump(4) << '\n';
        }

        // Save the current config to a file
        void saveToFile(const std::string& filePath) {
            try {
                std::ofstream file(filePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Could not open config file for writing: " + filePath);
                }
                file << config_.dump(4); // Pretty print with 4 spaces
                file.close();
            }
            catch (const std::exception& e) {
                std::cerr << "Error saving config: " << e.what() << '\n';
            }
        }

        // Apply labels to an external handler
        void applyLabels(const std::function<void(uint16_t, const std::string&)>& handler) const {
            try {
                if (config_.contains("labels")) {
                    for (const auto& [addressStr, label] : config_["labels"].items()) {
                        uint16_t address = static_cast<uint16_t>(std::stoul(addressStr, nullptr, 16));
                        handler(address, label);
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing labels: " << e.what() << '\n';
            }
        }

        // Add or update a label in the configuration
        void setLabel(uint16_t address, const std::string& label) {
            auto sAddress = fmt::format("{:#06x}", address);
            config_["labels"][sAddress] = label; 
        }

        // Remove a label from the configuration
        void removeLabel(uint16_t address) {
            auto sAddress = fmt::format("{:#06x}", address);
            config_["labels"].erase(sAddress);
        }

    private:
        nlohmann::json config_;

	};
}