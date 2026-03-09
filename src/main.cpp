#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "config/Config.h"
#include "communicator/Communicator.h"
#include <thread>
#include <chrono>

int main() {

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("agent.log", true);
    file_sink->set_level(spdlog::level::info);
    
    spdlog::logger logger("agent", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    
    spdlog::info("Web Agent starting...");

    // Загрузка конфига
    Config cfg;
    if (!cfg.loadFromFile("config.json")) {
        spdlog::error("Failed to load config. Exiting.");
        return 1;
    }

    spdlog::info("Loaded config: UID={}, server={}, interval={} sec", 
                 cfg.uid, cfg.server_url, cfg.poll_interval_sec);

    // Инициализация коммуникатора
    Communicator comm(cfg.server_url);

    // Если нет access_code - регистрируемся
    if (cfg.access_code.empty()) {
        spdlog::info("No access code, registering...");
        auto code = comm.registerAgent(cfg.uid);
        if (code.has_value()) {
            cfg.saveAccessCode(*code);
            spdlog::info("Access code saved.");
        } else {
            spdlog::error("Registration failed. Exiting.");
            return 1;
        }
    } else {
        spdlog::debug("Using existing access code: {}", cfg.access_code);
    }

    spdlog::info("Requesting task...");
    auto taskJson = comm.fetchTask(cfg.uid, cfg.access_code);
    if (taskJson.has_value()) {
        spdlog::info("Task response: {}", taskJson->dump(4));
    } else {
        spdlog::error("Failed to fetch task.");
    }

    return 0;
}
