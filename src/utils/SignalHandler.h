#pragma once
#include <functional>
#include <atomic>

class SignalHandler {
public:
    /**
     * @brief Initialize signal handler with shutdown callback
     * @param shutdownCallback Function to call on shutdown signal
     */
    static void init(std::function<void()> shutdownCallback);
    
    /**
     * @brief Check if shutdown signal was received
     * @return true if should stop, false otherwise
     */
    static bool shouldStop();
    
private:
    static std::atomic<bool> stopFlag_;
    static std::function<void()> callback_;
    
    /**
     * @brief Signal handler function
     * @param sig Signal number
     */
    static void handleSignal(int sig);
};
