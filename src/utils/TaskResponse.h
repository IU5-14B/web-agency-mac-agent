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
    
    // Флаги для удобства
    bool hasTask() const { return code == 1 && task.has_value(); }
    bool isWaiting() const { return code == 0; }
    bool isError() const { return code < 0; }
};

class TaskResponseParser {
public:
    // Парсит JSON от сервера и возвращает структурированный ответ
    static TaskResponse parse(const nlohmann::json& json);
    
    // Проверяет, корректен ли JSON (содержит нужные поля)
    static bool isValid(const nlohmann::json& json);
};
