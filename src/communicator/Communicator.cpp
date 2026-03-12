#include "Communicator.h"
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <random>

/**
 * @brief Constructor for Communicator
 * @param baseUrl Base URL of the server
 * @param mockMode Enable mock mode for testing
 */
Communicator::Communicator(const std::string& baseUrl, bool mockMode) 
    : baseUrl_(baseUrl), mockMode_(mockMode) {}

/**
 * @brief Enable or disable mock mode
 * @param enable true to enable mock mode, false to disable
 */
void Communicator::setMockMode(bool enable) {
    mockMode_ = enable;
    spdlog::info("Mock mode {}", enable ? "enabled" : "disabled");
}

/**
 * @brief Build full URL from endpoint
 * @param endpoint API endpoint path
 * @return Complete URL string
 */
std::string Communicator::buildUrl(const std::string& endpoint) const {
    if (baseUrl_.empty()) return endpoint;
    if (baseUrl_.back() == '/')
        return baseUrl_ + endpoint;
    else
        return baseUrl_ + "/" + endpoint;
}

/**
 * @brief Mock implementation of agent registration
 * @param uid Unique identifier of the agent
 * @return Mock access code
 */
std::optional<std::string> Communicator::mockRegister(const std::string& uid) {
    spdlog::info("[MOCK] Registering agent with UID: {}", uid);
    
    // Формат: xxxxxx-xxxx-xxxx-xxxx-xxxxxxxx (6-4-4-4-8 = 28 символов)
    // Без дефиса в конце!
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char* hex = "0123456789abcdef";
    
    std::stringstream ss;
    // Первая секция: 6 символов
    for (int i = 0; i < 6; i++) ss << hex[dis(gen)];
    ss << '-';
    
    // Вторая секция: 4 символа
    for (int i = 0; i < 4; i++) ss << hex[dis(gen)];
    ss << '-';
    
    // Третья секция: 4 символа
    for (int i = 0; i < 4; i++) ss << hex[dis(gen)];
    ss << '-';
    
    // Четвёртая секция: 4 символа
    for (int i = 0; i < 4; i++) ss << hex[dis(gen)];
    ss << '-';
    
    // Пятая секция: 8 символов (без дефиса в конце!)
    for (int i = 0; i < 8; i++) ss << hex[dis(gen)];
    
    std::string result = ss.str();
    spdlog::info("[MOCK] Registration successful, access code: {}", result);
    return result;
}

/**
 * @brief Mock implementation of task fetching
 * @param uid Unique identifier of the agent
 * @param accessCode Access code for authentication
 * @return Mock JSON task data
 */
std::optional<nlohmann::json> Communicator::mockFetchTask(const std::string& uid, const std::string& accessCode) {
    spdlog::info("[MOCK] Fetching task for UID: {}, access code: {}", uid, accessCode);
    
    mockTaskCounter_++;
    
    // Каждый третий раз возвращаем задание, иначе "нет задания"
    if (mockTaskCounter_ % 3 == 0) {
        // Генерируем session_id в формате UUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        const char* hex = "0123456789abcdef";
        
        std::string session_id(36, '-');
        // 8 символов
        for (int i = 0; i < 8; i++) session_id[i] = hex[dis(gen)];
        // 4 символа
        for (int i = 9; i < 13; i++) session_id[i] = hex[dis(gen)];
        // 4 символа
        for (int i = 14; i < 18; i++) session_id[i] = hex[dis(gen)];
        // 4 символа
        for (int i = 19; i < 23; i++) session_id[i] = hex[dis(gen)];
        // 12 символов
        for (int i = 24; i < 36; i++) session_id[i] = hex[dis(gen)];
        
        nlohmann::json task = {
            {"code_response", 1},
            {"task_code", "CONF"},
            {"options", "echo 'Hello from mock task'"},
            {"session_id", session_id},
            {"status", "RUN"}
        };
        spdlog::info("[MOCK] Task assigned: {}", task.dump());
        return task;
    } else {
        nlohmann::json response = {
            {"code_response", 0},
            {"status", "WAIT"}
        };
        spdlog::info("[MOCK] No task, waiting");
        return response;
    }
}

/**
 * @brief Mock implementation of result sending
 * @param uid Unique identifier of the agent
 * @param accessCode Access code for authentication
 * @param sessionId Session identifier
 * @param resultCode Result code of the task execution
 * @param message Result message
 * @param filesCount Number of files in the result
 * @param filePaths Paths to result files
 * @return true (always successful in mock mode)
 */
bool Communicator::mockSendResult(const std::string& uid, 
                                  const std::string& accessCode,
                                  const std::string& sessionId, 
                                  int resultCode,
                                  const std::string& message, 
                                  int filesCount,
                                  const std::vector<std::string>& filePaths) {
    
    spdlog::info("[MOCK] 📤 Sending result for session: {}", sessionId);
    spdlog::info("[MOCK]    result_code: {}, message: {}, files: {}", resultCode, message, filesCount);
    
    for (size_t i = 0; i < filePaths.size(); ++i) {
        spdlog::info("[MOCK]    file{}: {}", i+1, filePaths[i]);
    }
    
    // Всегда успех в mock-режиме
    return true;
}

/**
 * @brief Register agent with the server
 * @param uid Unique identifier of the agent
 * @param descr Description of the agent
 * @return Access code if registration successful, std::nullopt otherwise
 */
std::optional<std::string> Communicator::registerAgent(const std::string& uid, const std::string& descr) {
    if (mockMode_) {
        return mockRegister(uid);
    }
    
    nlohmann::json req;
    req["UID"] = uid;
    req["descr"] = descr;
    
    spdlog::debug("Registering agent with UID: {}", uid);
    
    auto response = cpr::Post(
        cpr::Url{buildUrl("wa_reg/")},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{req.dump()},
        cpr::Timeout{5000}
    );
    
    if (response.status_code != 200) {
        spdlog::error("Registration HTTP error: {}", response.status_code);
        return std::nullopt;
    }
    
    try {
        auto respJson = nlohmann::json::parse(response.text);
        int code = respJson.value("code_response", -1);
        if (code == 0) {
            std::string access = respJson.value("access_code", "");
            if (!access.empty()) {
                spdlog::info("Registration successful, access code: {}", access);
                return access;
            } else {
                spdlog::error("Registration success but no access_code");
            }
        } else {
            std::string msg = respJson.value("msg", "unknown error");
            spdlog::error("Registration failed: {} (code {})", msg, code);
        }
    } catch (const std::exception& e) {
        spdlog::error("JSON parse error in register: {}", e.what());
    }
    return std::nullopt;
}

/**
 * @brief Fetch task from the server
 * @param uid Unique identifier of the agent
 * @param accessCode Access code for authentication
 * @return JSON task data if successful, std::nullopt otherwise
 */
std::optional<nlohmann::json> Communicator::fetchTask(const std::string& uid, const std::string& accessCode) {
    if (mockMode_) {
        return mockFetchTask(uid, accessCode);
    }
    
    nlohmann::json req;
    req["UID"] = uid;
    req["descr"] = "web-agent";
    req["access_code"] = accessCode;
    
    spdlog::debug("Fetching task for UID: {}", uid);
    
    auto response = cpr::Post(
        cpr::Url{buildUrl("wa_task/")},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{req.dump()},
        cpr::Timeout{5000}
    );
    
    if (response.status_code != 200) {
        spdlog::error("Task fetch HTTP error: {}", response.status_code);
        return std::nullopt;
    }
    
    try {
        auto respJson = nlohmann::json::parse(response.text);
        
        spdlog::debug("Task response: {}", respJson.dump());
        
        // Проверяем наличие code_response (с учётом опечатки)
        int code = -1;
        if (respJson.contains("code_response")) {
            if (respJson["code_response"].is_string()) {
                code = std::stoi(respJson["code_response"].get<std::string>());
            } else {
                code = respJson["code_response"].get<int>();
            }
        } else if (respJson.contains("code_responce")) {
            if (respJson["code_responce"].is_string()) {
                code = std::stoi(respJson["code_responce"].get<std::string>());
            } else {
                code = respJson["code_responce"].get<int>();
            }
        }
        
        if (code == 0) {
            spdlog::info("No tasks available, status: {}", 
                         respJson.value("status", "UNKNOWN"));
        } else if (code == 1) {
            spdlog::info("Task received: {}", respJson.value("task_code", "UNKNOWN"));
        } else {
            spdlog::warn("Task fetch code: {}", code);
            if (respJson.contains("msg")) {
                spdlog::warn("Message: {}", respJson["msg"].get<std::string>());
            }
        }
        
        return respJson;
    } catch (const std::exception& e) {
        spdlog::error("JSON parse error in fetchTask: {}", e.what());
        return std::nullopt;
    }
}

/**
 * @brief Send task result to the server
 * @param uid Unique identifier of the agent
 * @param accessCode Access code for authentication
 * @param sessionId Session identifier
 * @param resultCode Result code of the task execution
 * @param message Result message
 * @param filesCount Number of files in the result
 * @param filePaths Paths to result files
 * @return true if result sent successfully, false otherwise
 */
bool Communicator::sendResult(const std::string& uid, 
                              const std::string& accessCode,
                              const std::string& sessionId, 
                              int resultCode,
                              const std::string& message, 
                              int filesCount,
                              const std::vector<std::string>& filePaths) {
    
    if (mockMode_) {
        return mockSendResult(uid, accessCode, sessionId, resultCode, message, filesCount, filePaths);
    }
    
    spdlog::info("📤 Sending result for session: {}", sessionId);
    spdlog::info("   result_code: {}, message: {}, files: {}", resultCode, message, filesCount);
    
    // 1. Формируем JSON для поля result
    nlohmann::json result_json;
    result_json["UID"] = uid;
    result_json["access_code"] = accessCode;
    result_json["message"] = message;
    result_json["files"] = filesCount;
    result_json["session_id"] = sessionId;
    
    std::string result_str = result_json.dump();
    spdlog::debug("Result JSON: {}", result_str);
    
    // 2. Создаём multipart запрос (правильный синтаксис CPR)
    cpr::Multipart multipart{
        {"result_code", std::to_string(resultCode)},
        {"result", result_str}
    };
    
    // 3. Добавляем файлы
    for (size_t i = 0; i < filePaths.size(); ++i) {
        std::string field_name = "file" + std::to_string(i + 1);
        const std::string& file_path = filePaths[i];
        
        // Проверяем, существует ли файл
        if (!std::filesystem::exists(file_path)) {
            spdlog::error("File not found: {}", file_path);
            continue;
        }
        
        spdlog::debug("Attaching file: {} as {}", file_path, field_name);
        
        // Добавляем файл в multipart (правильный синтаксис)
        multipart.parts.push_back({field_name, cpr::File{file_path}});
    }
    
    // 4. Отправляем запрос
    std::string url = buildUrl("wa_result/");
    spdlog::debug("POST to {}", url);
    
    auto response = cpr::Post(
        cpr::Url{url},
        multipart,
        cpr::Timeout{30000},  // 30 секунд на загрузку файлов
        cpr::VerifySsl{false}  // -k флаг (игнорировать SSL ошибки)
    );
    
    // 5. Проверяем ответ
    if (response.status_code != 200) {
        spdlog::error("HTTP error: {}", response.status_code);
        spdlog::error("Response: {}", response.text);
        return false;
    }
    
    spdlog::debug("Response: {}", response.text);
    
    // 6. Парсим ответ сервера
    try {
        auto resp_json = nlohmann::json::parse(response.text);
        
        // Обрабатываем опечатку в имени поля
        int code = -1;
        if (resp_json.contains("code_response")) {
            auto& val = resp_json["code_response"];
            if (val.is_string()) code = std::stoi(val.get<std::string>());
            else code = val.get<int>();
        } else if (resp_json.contains("code_responce")) {
            auto& val = resp_json["code_responce"];
            if (val.is_string()) code = std::stoi(val.get<std::string>());
            else code = val.get<int>();
        }
        
        if (code == 0) {
            spdlog::info("✅ Server accepted result");
            return true;
        } else {
            std::string msg = resp_json.value("msg", "Unknown error");
            spdlog::error("Server error: {} (code {})", msg, code);
            return false;
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse server response: {}", e.what());
        spdlog::error("Raw response: {}", response.text);
        return false;
    }
}