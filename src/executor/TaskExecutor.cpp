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
    : work_dir_(work_dir), results_dir_(results_dir), timeout_(std::chrono::seconds(60)) {
    
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

int TaskExecutor::runCommandWithTimeout(const std::string& command, std::string& output) {
    auto future = std::async(std::launch::async, [&command]() {
        return std::system(command.c_str());
    });
    
    auto status = future.wait_for(timeout_);
    
    if (status == std::future_status::ready) {
        int result = future.get();
        output = "Command completed with code: " + std::to_string(result);
        return result;
    } else if (status == std::future_status::timeout) {
        spdlog::error("Command timed out after {} seconds", timeout_.count());
        output = "Command timed out after " + std::to_string(timeout_.count()) + " seconds";
        return -3;
    } else {
        output = "Command execution error";
        return -4;
    }
}

int TaskExecutor::execute(const Task& task, std::string& message, std::vector<std::string>& out_files) {
    
    spdlog::info("Executing task: code={}, session={}", task.task_code, task.session_id);
    
    // 🔍 ДИАГНОСТИКА: логируем всё, что пришло
    spdlog::info("🔍 TASK DETAILS:");
    spdlog::info("   task_code: {}", task.task_code);
    spdlog::info("   options: '{}'", task.options);
    spdlog::info("   session_id: {}", task.session_id);
    spdlog::info("   status: {}", task.status);
    
    // Если это FILE — пока просто логируем и возвращаем успех
    if (task.task_code == "FILE") {
        std::string filename = task.options;  // "test.txt"
        spdlog::info("📁 Looking for file: {}", filename);
        
        // Ищем файл в results_dir_ (там обычно лежат результаты)
        fs::path file_path = fs::path(results_dir_) / filename;
        
        if (!fs::exists(file_path)) {
            // Если нет в results, ищем в work_dir_
            file_path = fs::path(work_dir_) / filename;
        }
        
        if (fs::exists(file_path)) {
            spdlog::info("✅ Found file: {}", file_path.string());
            out_files.push_back(file_path.string());
            message = "File found and ready to send";
            return 0;
        } else {
            spdlog::error("❌ File not found: {}", filename);
            message = "File not found: " + filename;
            return -2;  // файл не найден
        }
    }
    
    // Дальше идёт существующая логика для CONF/TASK...
    if (task.task_code != "CONF" && task.task_code != "TASK") {
        message = "Unsupported task code: " + task.task_code;
        spdlog::error(message);
        return -1;
    }
    
    if (task.options.empty()) {
        spdlog::info("Empty options, task completed without action");
        message = "Task completed (no action)";
        out_files = collectResultFiles();
        return 0;
    }
    
    // Запоминаем текущую директорию
    auto original_path = fs::current_path();
    
    try {
        // Переходим в рабочую папку
        fs::current_path(work_dir_);
        spdlog::debug("Changed working directory to: {}", work_dir_);
        
        // Запускаем команду
        spdlog::info("Running command with {}s timeout: {}", timeout_.count(), task.options);
        
        std::string cmd_output;
        int result = runCommandWithTimeout(task.options, cmd_output);
        
        // Возвращаемся в исходную папку
        fs::current_path(original_path);
        
        if (result == 0) {
            spdlog::info("Command executed successfully");
            
            // Копируем все файлы из work в results
            int copied = 0;
            for (const auto& entry : fs::directory_iterator(work_dir_)) {
                if (entry.is_regular_file()) {
                    fs::path dest = results_dir_ / entry.path().filename();
                    fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
                    spdlog::debug("Copied {} to results", entry.path().filename().string());
                    copied++;
                }
            }
            
            // Собираем файлы из results для отправки
            out_files = collectResultFiles();
            
            if (out_files.empty()) {
                spdlog::warn("No result files found in {}", results_dir_);
                message = "Command executed, but no result files";
            } else {
                spdlog::info("Collected {} result files", out_files.size());
                message = "Command executed successfully, " + std::to_string(out_files.size()) + " file(s) created";
            }
            
            // Очищаем work папку
            for (const auto& entry : fs::directory_iterator(work_dir_)) {
                fs::remove(entry.path());
            }
            spdlog::debug("Cleaned work directory");
            
            return 0;
            
        } else if (result == -3) {
            spdlog::error("Command timed out after {} seconds", timeout_.count());
            message = "Command timed out after " + std::to_string(timeout_.count()) + " seconds";
            fs::current_path(original_path);
            return -3;
        } else {
            spdlog::error("Command failed with code: {}", result);
            message = "Command failed with code: " + std::to_string(result);
            fs::current_path(original_path);
            return -2;
        }
        
    } catch (const std::exception& e) {
        fs::current_path(original_path);
        spdlog::error("Exception during command execution: {}", e.what());
        message = "Exception: " + std::string(e.what());
        return -4;
    }
}
