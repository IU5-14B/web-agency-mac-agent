#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "config/Config.h"

struct Task {
    /** @brief Task code (type of task, e.g. "TASK", "FILE") */
    std::string task_code;
    /** @brief Options or command string associated with the task */
    std::string options;
    /** @brief Session identifier for the task */
    std::string session_id;
    /** @brief Task status ("RUN", "WAIT" and similar) */
    std::string status;
};

class TaskExecutor {
public:
    /**
     * @brief Construct a TaskExecutor
     * @param work_dir Directory where tasks are executed
     * @param results_dir Directory where result files are stored
     * @param config Optional pointer to runtime `Config` object
     */
    TaskExecutor(const std::string& work_dir, const std::string& results_dir, Config* config = nullptr);

    /**
     * @brief Execute given task
     * @param task Task description to execute
     * @param[out] message Human-readable result or error message
     * @param[out] out_files List of paths to files produced for sending
     * @return 0 on success, negative value on error, or platform-specific exit code
     */
    int execute(const Task& task, std::string& message, std::vector<std::string>& out_files);

    /**
     * @brief Set maximum execution timeout for tasks
     * @param timeout Timeout duration as std::chrono::seconds
     */
    void setTimeout(std::chrono::seconds timeout);

    /**
     * @brief Set pointer to configuration object
     * @param config Pointer to `Config` instance used to persist changes
     */
    void setConfig(Config* config);
    
private:
    std::string work_dir_;
    std::string results_dir_;
    std::chrono::seconds timeout_ = std::chrono::seconds(30);
    Config* config_ = nullptr;

    /**
     * @brief Collect result files from the `results_dir_`
     * @return Vector with full paths to result files
     */
    std::vector<std::string> collectResultFiles();

    /**
     * @brief Run a shell command with timeout
     * @param command Command string to execute
     * @param[out] output Short textual description of execution outcome
     * @return 0 on success, negative codes for errors, or exit code of the command
     */
    int runCommandWithTimeout(const std::string& command, std::string& output);
};
