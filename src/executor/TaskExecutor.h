#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "config/Config.h"

struct Task {
    std::string task_code;
    std::string options;
    std::string session_id;
    std::string status;
};

class TaskExecutor {
public:
    // Конструктор с Config (новый)
    TaskExecutor(const std::string& work_dir, const std::string& results_dir, Config* config = nullptr);
    
    // Старый конструктор (можно оставить для обратной совместимости)
    // TaskExecutor(const std::string& work_dir, const std::string& results_dir);
    
    // Выполнить задание
    int execute(const Task& task, std::string& message, std::vector<std::string>& out_files);

    void setTimeout(std::chrono::seconds timeout);
    
    // Новый метод для установки Config
    void setConfig(Config* config);
    
private:
    std::string work_dir_;
    std::string results_dir_;
    std::chrono::seconds timeout_ = std::chrono::seconds(30);
    Config* config_ = nullptr;

    std::vector<std::string> collectResultFiles();
    int runCommandWithTimeout(const std::string& command, std::string& output);
};
