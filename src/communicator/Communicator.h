#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class Communicator {
public:
    /**
     * @brief Constructor for Communicator
     * @param baseUrl Base URL of the server
     * @param mockMode Enable mock mode for testing (default: false)
     */
    Communicator(const std::string& baseUrl, bool mockMode = false);
    
    /**
     * @brief Enable or disable mock mode
     * @param enable true to enable mock mode, false to disable
     */
    void setMockMode(bool enable);
    
    /**
     * @brief Build full URL from endpoint
     * @param endpoint API endpoint path
     * @return Complete URL string
     */
    std::string buildUrl(const std::string& endpoint) const;
    
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
    bool sendResult(const std::string& uid, 
                    const std::string& accessCode,
                    const std::string& sessionId, 
                    int resultCode,
                    const std::string& message, 
                    int filesCount,
                    const std::vector<std::string>& filePaths);
    
private:
    std::string baseUrl_;
    bool mockMode_;
    int mockTaskCounter_ = 0;  // для генерации разных заданий
    
    
    /**
     * @brief Mock implementation of agent registration
     * @param uid Unique identifier of the agent
     * @return Mock access code
     */
    std::optional<std::string> mockRegister(const std::string& uid);
    
    /**
     * @brief Mock implementation of task fetching
     * @param uid Unique identifier of the agent
     * @param accessCode Access code for authentication
     * @return Mock JSON task data
     */
    std::optional<nlohmann::json> mockFetchTask(const std::string& uid, const std::string& accessCode);
    
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
    bool mockSendResult(const std::string& uid, const std::string& accessCode,
                       const std::string& sessionId, int resultCode,
                       const std::string& message, int filesCount,
                       const std::vector<std::string>& filePaths);
};
