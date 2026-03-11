#include "SignalHandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

std::atomic<bool> SignalHandler::stopFlag_{false};
std::function<void()> SignalHandler::callback_{nullptr};

/**
 * @brief Initialize signal handler with shutdown callback
 * @param shutdownCallback Function to call on shutdown signal
 */
void SignalHandler::init(std::function<void()> shutdownCallback) {
    callback_ = shutdownCallback;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
}

/**
 * @brief Check if shutdown signal was received
 * @return true if should stop, false otherwise
 */
bool SignalHandler::shouldStop() {
    return stopFlag_;
}

/**
 * @brief Signal handler function
 * @param sig Signal number
 */
void SignalHandler::handleSignal(int sig) {
    spdlog::warn("Received signal {}, shutting down...", sig);
    stopFlag_ = true;
    if (callback_) {
        callback_();
    }
}
