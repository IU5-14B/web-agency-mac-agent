#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class Communicator {
public:
    Communicator(const std::string& baseUrl, bool mockMode = false);
    
    // Включить/выключить режим заглушки
    void setMockMode(bool enable);
    
    // Регистрация агента
    std::optional<std::string> registerAgent(const std::string& uid, const std::string& descr = "web-agent");
    
    // Запрос задания
    std::optional<nlohmann::json> fetchTask(const std::string& uid, const std::string& accessCode);
    
    // Отправка результата
    bool sendResult(const std::string& uid, const std::string& accessCode,
                    const std::string& sessionId, int resultCode,
                    const std::string& message, int filesCount,
                    const std::vector<std::string>& filePaths);
    
private:
    std::string baseUrl_;
    bool mockMode_;
    int mockTaskCounter_ = 0;  // для генерации разных заданий
    
    std::string buildUrl(const std::string& endpoint) const;
    
    // Заглушки
    std::optional<std::string> mockRegister(const std::string& uid);
    std::optional<nlohmann::json> mockFetchTask(const std::string& uid, const std::string& accessCode);
    bool mockSendResult(const std::string& uid, const std::string& accessCode,
                       const std::string& sessionId, int resultCode,
                       const std::string& message, int filesCount,
                       const std::vector<std::string>& filePaths);
};
