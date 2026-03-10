#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class Communicator {
public:
    /**
     * @brief Constructor for Communicator
     * @param baseUrl Base URL of the server
     */
    Communicator(const std::string& baseUrl);
    
    /**
     * @brief Register agent with the server
     * @param uid Unique identifier of the agent
     * @param descr Description of the agent (default: "web-agent")
     * @return Access code if registration successful, std::nullopt otherwise
     */
    std::optional<std::string> registerAgent(const std::string& uid, const std::string& descr = "web-agent");
    
    /**
     * @brief Fetch task from the server
     * @param uid Unique identifier of the agent
     * @param accessCode Access code for authentication
     * @return JSON task data if successful, std::nullopt otherwise
     */
    std::optional<nlohmann::json> fetchTask(const std::string& uid, const std::string& accessCode);
    
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
    bool sendResult(const std::string& uid, const std::string& accessCode,
                    const std::string& sessionId, int resultCode,
                    const std::string& message, int filesCount,
                    const std::vector<std::string>& filePaths);
    
private:
    std::string baseUrl_;
    
    /**
     * @brief Build full URL from endpoint
     * @param endpoint API endpoint path
     * @return Complete URL string
     */
    std::string buildUrl(const std::string& endpoint) const;
};
