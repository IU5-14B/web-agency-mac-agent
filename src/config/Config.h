#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct Config {
    std::string uid;
    std::string server_url;
    int poll_interval_sec = 10;
    std::string work_dir;
    std::string results_dir;
    std::string log_file;
    std::string log_level = "info";
    std::string access_code;

    /**
     * @brief Load configuration from file
     * @param path Path to configuration file
     * @return true if loading successful, false otherwise
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * @brief Save access code to configuration
     * @param code Access code to save
     */
    void saveAccessCode(const std::string& code);
};
