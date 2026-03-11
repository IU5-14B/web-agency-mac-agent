#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Config {
public:
    std::string uid;
    std::string server_url;
    int poll_interval_sec = 10;
    std::string work_dir;
    std::string results_dir;
    std::string log_file;
    std::string log_level = "info";
    std::string access_code;

    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    void saveAccessCode(const std::string& code);

private:
    std::string last_config_path_;
};
