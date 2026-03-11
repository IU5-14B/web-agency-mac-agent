#include "Communicator.h"
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>
#include <random>

Communicator::Communicator(const std::string& baseUrl, bool mockMode) 
    : baseUrl_(baseUrl), mockMode_(mockMode) {}

void Communicator::setMockMode(bool enable) {
    mockMode_ = enable;
    spdlog::info("Mock mode {}", enable ? "enabled" : "disabled");
}

std::string Communicator::buildUrl(const std::string& endpoint) const {
    if (baseUrl_.empty()) return endpoint;
    if (baseUrl_.back() == '/')
        return baseUrl_ + endpoint;
    else
        return baseUrl_ + "/" + endpoint;
}

// Заглушка для регистрации
std::optional<std::string> Communicator::mockRegister(const std::string& uid) {
    spdlog::info("[MOCK] Registering agent with UID: {}", uid);
    
    // Генерируем случайный access_code
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex_digits = "0123456789abcdef";
    std::string code(32, '0');
    for (int i = 0; i < 32; ++i) {
        code[i] = hex_digits[dis(gen)];
    }
    // Форматируем как в примере: "594807-1ddb-36af-9616-d8ed2b9d"
    std::string formatted = code.substr(0, 6) + "-" + code.substr(6, 4) + "-" +
                           code.substr(10, 4) + "-" + code.substr(14, 4) + "-" +
                           code.substr(18, 8);
    
    spdlog::info("[MOCK] Registration successful, access code: {}", formatted);
    return formatted;
}

// Заглушка для получения задания
std::optional<nlohmann::json> Communicator::mockFetchTask(const std::string& uid, const std::string& accessCode) {
    spdlog::info("[MOCK] Fetching task for UID: {}, access code: {}", uid, accessCode);
    
    mockTaskCounter_++;
    
    // Каждый третий раз возвращаем задание, иначе "нет задания"
    if (mockTaskCounter_ % 3 == 0) {
        // Генерируем session_id
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        const char* chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string session_id(36, '0');
        for (int i = 0; i < 36; ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                session_id[i] = '-';
            } else {
                session_id[i] = chars[dis(gen) % 62];
            }
        }
        
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

// Заглушка для отправки результата
bool Communicator::mockSendResult(const std::string& uid, const std::string& accessCode,
                                 const std::string& sessionId, int resultCode,
                                 const std::string& message, int filesCount,
                                 const std::vector<std::string>& filePaths) {
    spdlog::info("[MOCK] Sending result for session: {}", sessionId);
    spdlog::info("[MOCK]   result_code: {}, message: {}, files: {}", resultCode, message, filesCount);
    
    for (size_t i = 0; i < filePaths.size(); ++i) {
        spdlog::info("[MOCK]   file{}: {}", i+1, filePaths[i]);
    }
    
    // Всегда успех
    return true;
}

// Оригинальные методы с выбором режима
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

bool Communicator::sendResult(const std::string& uid, const std::string& accessCode,
                              const std::string& sessionId, int resultCode,
                              const std::string& message, int filesCount,
                              const std::vector<std::string>& filePaths) {
    if (mockMode_) {
        return mockSendResult(uid, accessCode, sessionId, resultCode, message, filesCount, filePaths);
    }
    
    // Здесь будет реальная отправка
    spdlog::warn("Real sendResult not implemented yet");
    return false;
}
