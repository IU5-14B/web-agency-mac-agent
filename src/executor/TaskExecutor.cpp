#include "TaskExecutor.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <thread>
#include <future>
#include <chrono>

namespace fs = std::filesystem;

TaskExecutor::TaskExecutor(const std::string& work_dir, const std::string& results_dir)
    : work_dir_(work_dir), results_dir_(results_dir), timeout_(std::chrono::seconds(30)) {
    
    fs::create_directories(work_dir_);
    fs::create_directories(results_dir_);
    
    spdlog::debug("TaskExecutor initialized: work_dir={}, results_dir={}, timeout={}s", 
                  work_dir_, results_dir_, timeout_.count());
}

void TaskExecutor::setTimeout(std::chrono::seconds timeout) {
    timeout_ = timeout;
    spdlog::debug("TaskExecutor timeout set to {} seconds", timeout_.count());
}

std::vector<std::string> TaskExecutor::collectResultFiles() {
    std::vector<std::string> files;
    
    if (!fs::exists(results_dir_)) {
        return files;
    }
    
    for (const auto& entry : fs::directory_iterator(results_dir_)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
            spdlog::debug("Found result file: {}", entry.path().filename().string());
        }
    }
    
    return files;
}

// Новый метод: запуск команды с таймаутом
int TaskExecutor::runCommandWithTimeout(const std::string& command, std::string& output) {
    // Запускаем команду в асинхронном future
    auto future = std::async(std::launch::async, [&command]() {
        return std::system(command.c_str());
    });
    
    // Ждём результат с таймаутом
    auto status = future.wait_for(timeout_);
    
    if (status == std::future_status::ready) {
        // Команда завершилась вовремя
        int result = future.get();
        output = "Command completed with code: " + std::to_string(result);
        return result;
    } else if (status == std::future_status::timeout) {
        // Таймаут!
        spdlog::error("Command timed out after {} seconds", timeout_.count());
        output = "Command timed out after " + std::to_string(timeout_.count()) + " seconds";
        return -3;  // специальный код для таймаута
    } else {
        // deferred (не должно случиться)
        output = "Command execution error";
        return -4;
    }
}

int TaskExecutor::execute(const Task& task, std::string& message, std::vector<std::string>& out_files) {
    
    spdlog::info("Executing task: code={}, session={}", task.task_code, task.session_id);
    
    if (task.task_code != "CONF") {
        message = "Unsupported task code: " + task.task_code;
        spdlog::error(message);
        return -1;
    }
    
    if (task.options.empty()) {
        spdlog::info("Empty options, task completed without action");
        message = "Task completed (no action)";
        
        // Проверяем, может файлы уже есть
        out_files = collectResultFiles();
        if (!out_files.empty()) {
            spdlog::info("Found {} existing result files", out_files.size());
        }
        
        return 0;
    }
    
    spdlog::info("Running command with {}s timeout: {}", timeout_.count(), task.options);
    
    // Запускаем с таймаутом
    std::string cmd_output;
    int result = runCommandWithTimeout(task.options, cmd_output);
    
    if (result == 0) {
        spdlog::info("Command executed successfully");
        
        // Собираем файлы результатов
        out_files = collectResultFiles();
        
        if (out_files.empty()) {
            spdlog::warn("No result files found in {}", results_dir_);
            message = "Command executed, but no result files";
        } else {
            spdlog::info("Collected {} result files", out_files.size());
            message = "Command executed successfully, " + std::to_string(out_files.size()) + " file(s) created";
        }
        
        return 0;
        
    } else if (result == -3) {
        // Таймаут
        message = "Command timed out after " + std::to_string(timeout_.count()) + " seconds";
        
        // Даже при таймауте, могли появиться файлы?
        out_files = collectResultFiles();
        if (!out_files.empty()) {
            spdlog::warn("Found {} files despite timeout", out_files.size());
        }
        
        return -3;
        
    } else {
        // Другая ошибка
        spdlog::error("Command failed with code: {}", result);
        message = "Command failed with code: " + std::to_string(result);
        
        // Даже при ошибке, могли появиться файлы?
        out_files = collectResultFiles();
        if (!out_files.empty()) {
            spdlog::warn("Found {} files despite command failure", out_files.size());
        }
        
        return -2;
    }
}
