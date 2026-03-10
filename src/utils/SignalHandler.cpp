#include "SignalHandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

std::atomic<bool> SignalHandler::stopFlag_{false};
std::function<void()> SignalHandler::callback_{nullptr};

void SignalHandler::init(std::function<void()> shutdownCallback) {
    callback_ = shutdownCallback;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
}

bool SignalHandler::shouldStop() {
    return stopFlag_;
}

void SignalHandler::handleSignal(int sig) {
    spdlog::warn("Received signal {}, shutting down...", sig);
    stopFlag_ = true;
    if (callback_) {
        callback_();
    }
}
