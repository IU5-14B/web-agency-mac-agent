#include "Communicator.h"
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>

/**
 * @brief Constructor for Communicator
 * @param baseUrl Base URL of the server
 */
Communicator::Communicator(const std::string& baseUrl) : baseUrl_(baseUrl) {}

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
 * @brief Register agent with the server
 * @param uid Unique identifier of the agent
 * @param descr Description of the agent
 * @return Access code if registration successful, std::nullopt otherwise
 */
std::optional<std::string> Communicator::registerAgent(const std::string& uid, const std::string& descr) {
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
bool Communicator::sendResult(const std::string& uid, const std::string& accessCode,
                              const std::string& sessionId, int resultCode,
                              const std::string& message, int filesCount,
                              const std::vector<std::string>& filePaths) {
    spdlog::warn("sendResult not implemented yet");
    return true;
}
