#include "TaskResponse.h"
#include <spdlog/spdlog.h>

TaskResponse TaskResponseParser::parse(const nlohmann::json& json) {
    TaskResponse response;
    response.code = -1;
    
    try {
        // Извлекаем code_response (с учётом опечатки)
        if (json.contains("code_response")) {
            const auto& val = json["code_response"];
            if (val.is_string()) response.code = std::stoi(val.get<std::string>());
            else response.code = val.get<int>();
        } else if (json.contains("code_responce")) {
            const auto& val = json["code_responce"];
            if (val.is_string()) response.code = std::stoi(val.get<std::string>());
            else response.code = val.get<int>();
        } else {
            spdlog::error("JSON missing code_response field");
            response.message = "Missing code_response";
            return response;
        }
        
        // Извлекаем status если есть
        if (json.contains("status")) {
            response.status = json["status"].get<std::string>();
        }
        
        // Извлекаем сообщение если есть
        if (json.contains("msg")) {
            response.message = json["msg"].get<std::string>();
        }
        
        // Если это задание (code == 1), парсим детали
        if (response.code == 1) {
            TaskInfo task;
            task.code_response = response.code;
            task.task_code = json.value("task_code", "");
            task.options = json.value("options", "");
            task.session_id = json.value("session_id", "");
            task.status = json.value("status", "RUN");
            
            // Валидация обязательных полей
            if (task.task_code.empty() || task.session_id.empty()) {
                spdlog::error("Task missing required fields");
                response.message = "Invalid task format";
                response.code = -5;  // Свой код ошибки
                return response;
            }
            
            response.task = task;
            spdlog::debug("Parsed task: code={}, session={}", 
                         task.task_code, task.session_id);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Error parsing task response: {}", e.what());
        response.code = -4;  // Ошибка парсинга
        response.message = "Parse error: " + std::string(e.what());
    }
    
    return response;
}

bool TaskResponseParser::isValid(const nlohmann::json& json) {
    return json.contains("code_response") || json.contains("code_responce");
}
