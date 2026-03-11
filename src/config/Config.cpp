#include "Config.h"
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>

bool Config::loadFromFile(const std::string& path) {
    last_config_path_ = path;  // запоминаем путь

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("Cannot open config file: {}", path);
        return false;
    }

    try {
        nlohmann::json data = nlohmann::json::parse(file);
        
        uid = data.value("uid", "");
        server_url = data.value("server_url", "");
        poll_interval_sec = data.value("poll_interval_sec", 5);
        work_dir = data.value("work_dir", "./work");
        results_dir = data.value("results_dir", "./results");
        log_file = data.value("log_file", "./agent.log");
        log_level = data.value("log_level", "info");
        access_code = data.value("access_code", "");
        
        spdlog::debug("Config loaded: UID={}, server={}, interval={}", 
                     uid, server_url, poll_interval_sec);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse config: {}", e.what());
        return false;
    }
}

bool Config::saveToFile(const std::string& path) {  // с параметром!
    nlohmann::json data;
    data["uid"] = uid;
    data["server_url"] = server_url;
    data["poll_interval_sec"] = poll_interval_sec;
    data["work_dir"] = work_dir;
    data["results_dir"] = results_dir;
    data["log_file"] = log_file;
    data["log_level"] = log_level;
    data["access_code"] = access_code;

    std::ofstream file(path);
    if (!file.is_open()) {
        spdlog::error("Cannot write config to {}", path);
        return false;
    }

    file << data.dump(4);
    spdlog::debug("Config saved to {}", path);
    return true;
}

void Config::saveAccessCode(const std::string& code) {
    access_code = code;
    if (!last_config_path_.empty()) {
        saveToFile(last_config_path_);  // используем сохранённый путь
        spdlog::info("Access code saved to {}", last_config_path_);
    } else {
        // Если путь не известен, сохраняем в config.json по умолчанию
        saveToFile("config.json");
        spdlog::warn("No last path, saved to config.json");
    }
}
