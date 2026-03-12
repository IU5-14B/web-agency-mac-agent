#pragma once

#include <string>
#include <vector>

struct Task {
    std::string task_code;
    std::string options;
    std::string session_id;
    std::string status;
};

class TaskExecutor {
public:
    TaskExecutor(const std::string& work_dir, const std::string& results_dir);
    
    // Выполнить задание
    // Возвращает: 0 - успех, отрицательное число - код ошибки
    int execute(const Task& task, std::string& message, std::vector<std::string>& out_files);

    void setTimeout(std::chrono::seconds timeout);
    
private:
    std::string work_dir_;
    std::string results_dir_;
    std::chrono::seconds timeout_ = std::chrono::seconds(30);

    std::vector<std::string> collectResultFiles();

    int runCommandWithTimeout(const std::string& command, std::string& output);
};
