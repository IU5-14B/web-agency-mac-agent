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

/**
 * @brief Construct a TaskExecutor
 * @param work_dir Directory where tasks are executed
 * @param results_dir Directory to store result files
 * @param config Optional pointer to Config for runtime changes
 */
TaskExecutor::TaskExecutor(const std::string& work_dir, const std::string& results_dir, Config* config)
    : work_dir_(work_dir), results_dir_(results_dir), config_(config), timeout_(std::chrono::seconds(60)) {
    
    fs::create_directories(work_dir_);
    fs::create_directories(results_dir_);
    
    spdlog::debug("TaskExecutor initialized: work_dir={}, results_dir={}, timeout={}s", 
                  work_dir_, results_dir_, timeout_.count());
}

/**
 * @brief Set pointer to runtime Config
 * @param config Pointer to Config instance
 */
void TaskExecutor::setConfig(Config* config) {
    config_ = config;
    spdlog::debug("Config pointer set in TaskExecutor");
}

/**
 * @brief Set task execution timeout
 * @param timeout Timeout duration in seconds
 */
void TaskExecutor::setTimeout(std::chrono::seconds timeout) {
    timeout_ = timeout;
    spdlog::debug("TaskExecutor timeout set to {} seconds", timeout_.count());
}

/**
 * @brief Collect all regular files from the results directory
 * @return Vector of file paths found in `results_dir_`
 */
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

/**
 * @brief Execute a shell command and enforce the configured timeout
 * @param command Command string to run
 * @param[out] output Short description of the execution result
 * @return Process exit code on success, negative on error/timeouts
 */
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

/**
 * @brief Execute a Task based on its `task_code`
 * @param task Task object describing the action to perform
 * @param[out] message Human-readable message describing outcome
 * @param[out] out_files Collected result file paths for sending
 * @return 0 on success, negative on error or specific failure codes
 */
int TaskExecutor::execute(const Task& task, std::string& message, std::vector<std::string>& out_files) {
    
    spdlog::info("Executing task: code={}, session={}", task.task_code, task.session_id);
    
    // ДИАГНОСТИКА: логируем всё, что пришло
    spdlog::info("🔍 TASK DETAILS:");
    spdlog::info("   task_code: {}", task.task_code);
    spdlog::info("   options: '{}'", task.options);
    spdlog::info("   session_id: {}", task.session_id);
    spdlog::info("   status: {}", task.status);
    
    // ============================================================
    // ТИП 1: FILE — найти и отправить файл
    // ============================================================
    if (task.task_code == "FILE") {
        std::string filename = task.options;
        spdlog::info("📁 Looking for file: {}", filename);
        
        // Ищем файл в results_dir_
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
            return -2;
        }
    }
    
    // ============================================================
    // ТИП 2: TIMEOUT — изменить интервал опроса сервера (poll interval)
    // ============================================================
    if (task.task_code == "TIMEOUT") {
        spdlog::info("TIMEOUT task received (change poll interval)");
        
        int new_interval = 0;
        try {
            new_interval = std::stoi(task.options);
        } catch (...) {
            message = "Invalid interval value: " + task.options;
            spdlog::error(message);
            return -2;
        }
        
        if (config_) {
            // Меняем интервал опроса в Config
            config_->poll_interval_sec = new_interval;
            
            // Сохраняем в файл
            config_->saveToFile(config_->getLastConfigPath());
            
            message = "Poll interval changed to " + std::to_string(new_interval) + " seconds";
            spdlog::info("✅ {}", message);
        } else {
            message = "Cannot change poll interval: config not available";
            spdlog::error(message);
            return -3;
        }
        
        return 0;
    }
    
    // ============================================================
    // ТИП 3: CONF — изменить максимальное время выполнения задачи (task timeout)
    // ============================================================
    if (task.task_code == "CONF") {
        spdlog::info("CONF task received (change task timeout)");
        
        int new_timeout = 0;
        try {
            new_timeout = std::stoi(task.options);
        } catch (...) {
            message = "Invalid timeout value: " + task.options;
            spdlog::error(message);
            return -2;
        }
        
        // Меняем таймаут выполнения задачи
        setTimeout(std::chrono::seconds(new_timeout));
        
        message = "Task timeout changed to " + std::to_string(new_timeout) + " seconds";
        spdlog::info("✅ {}", message);
        
        return 0;
    }
    
    // ============================================================
    // ТИП 4: CONFIG — изменить другие конфигурационные параметры
    // ============================================================
    if (task.task_code == "CONFIG") {
        spdlog::info("CONFIG task received");
        
        // Ожидаем формат "parameter=value"
        std::string options = task.options;
        size_t eq_pos = options.find('=');
        
        if (eq_pos == std::string::npos) {
            message = "Invalid config format. Expected 'parameter=value'";
            spdlog::error(message);
            return -2;
        }
        
        std::string param = options.substr(0, eq_pos);
        std::string value = options.substr(eq_pos + 1);
        
        spdlog::info("   param: {}, value: {}", param, value);
        
        if (config_) {
            if (param == "poll_interval") {
                try {
                    int new_interval = std::stoi(value);
                    config_->poll_interval_sec = new_interval;
                    config_->saveToFile(config_->getLastConfigPath());
                    message = "Poll interval changed to " + value + " seconds";
                } catch (...) {
                    message = "Invalid interval value: " + value;
                    spdlog::error(message);
                    return -2;
                }
            } else if (param == "log_level") {
                config_->log_level = value;
                config_->saveToFile(config_->getLastConfigPath());
                message = "Log level changed to " + value;
            } else {
                message = "Unknown config parameter: " + param;
                spdlog::error(message);
                return -2;
            }
            
            spdlog::info("✅ {}", message);
            return 0;
        } else {
            message = "Cannot change config: config not available";
            spdlog::error(message);
            return -3;
        }
    }
    
    // ============================================================
    // ТИП 5: TASK — запустить программу
    // ============================================================
    if (task.task_code == "TASK") {
        
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
                for (const auto& entry : fs::directory_iterator(work_dir_)) {
                    if (entry.is_regular_file()) {
                        fs::path dest = results_dir_ / entry.path().filename();
                        fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
                        spdlog::debug("Copied {} to results", entry.path().filename().string());
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
    
    // ============================================================
    // Неподдерживаемый тип задачи
    // ============================================================
    message = "Unsupported task code: " + task.task_code;
    spdlog::error(message);
    return -1;
}
