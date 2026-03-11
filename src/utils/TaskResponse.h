#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

struct TaskInfo {
    std::string task_code;
    std::string options;
    std::string session_id;
    std::string status;
    int code_response;
};

struct TaskResponse {
    int code;              // code_response (0, 1, отрицательный)
    std::string status;    // "WAIT", "RUN" и т.д.
    std::optional<TaskInfo> task;  // заполнено если code == 1
    std::string message;   // сообщение об ошибке если есть
    
    /**
     * @brief Check if response contains a task
     * @return true if task is available, false otherwise
     */
    bool hasTask() const { return code == 1 && task.has_value(); }
    
    /**
     * @brief Check if response indicates waiting state
     * @return true if waiting, false otherwise
     */
    bool isWaiting() const { return code == 0; }
    
    /**
     * @brief Check if response indicates an error
     * @return true if error, false otherwise
     */
    bool isError() const { return code < 0; }
};

class TaskResponseParser {
public:
    /**
     * @brief Parse JSON response from server
     * @param json JSON object from server
     * @return Structured task response
     */
    static TaskResponse parse(const nlohmann::json& json);
    
    /**
     * @brief Validate JSON response structure
     * @param json JSON object to validate
     * @return true if JSON is valid, false otherwise
     */
    static bool isValid(const nlohmann::json& json);
};
