#pragma once
#include <functional>
#include <atomic>

class SignalHandler {
public:
    static void init(std::function<void()> shutdownCallback);
    static bool shouldStop();
    
private:
    static std::atomic<bool> stopFlag_;
    static std::function<void()> callback_;
    static void handleSignal(int sig);
};
