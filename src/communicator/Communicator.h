#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class Communicator {
public:
    Communicator(const std::string& baseUrl);
    
    // Регистрация агента
    std::optional<std::string> registerAgent(const std::string& uid, const std::string& descr = "web-agent");
    
    // Запрос задания
    std::optional<nlohmann::json> fetchTask(const std::string& uid, const std::string& accessCode);
    
    // Отправка результата (пока заглушка)
    bool sendResult(const std::string& uid, const std::string& accessCode,
                    const std::string& sessionId, int resultCode,
                    const std::string& message, int filesCount,
                    const std::vector<std::string>& filePaths);
    
private:
    std::string baseUrl_;
    std::string buildUrl(const std::string& endpoint) const;
};
