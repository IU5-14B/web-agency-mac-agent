#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "config/Config.h"
#include "communicator/Communicator.h"
#include "utils/SignalHandler.h"
#include "utils/TaskResponse.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Настройка логгера
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("agent.log", true);
    file_sink->set_level(spdlog::level::info);
    
    spdlog::logger logger("agent", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    
    spdlog::info("Web Agent starting...");

    // Получаем путь к конфигу из аргументов командной строки
    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
        spdlog::info("Using config from command line: {}", config_path);
    } else {
        // Ищем конфиг в стандартных местах
        if (fs::exists("config/config.json")) {
            config_path = "config/config.json";
        } else if (fs::exists("../config/config.json")) {
            config_path = "../config/config.json";
        }
    }

    // Загрузка конфига
    Config cfg;
    if (!cfg.loadFromFile(config_path)) {
        spdlog::error("Failed to load config. Exiting.");
        return 1;
    }

    spdlog::info("Loaded config: UID={}, server={}, interval={} sec", 
                 cfg.uid, cfg.server_url, cfg.poll_interval_sec);

    // Инициализация обработчика сигналов
    SignalHandler::init([]() {
        spdlog::info("Shutdown requested, cleaning up...");
    });

    // Инициализация коммуникатора (реальный режим)
    Communicator comm(cfg.server_url, false);

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

    // Основной цикл
    spdlog::info("Entering main loop. Press Ctrl+C to stop.");
    int error_count = 0;
    const int max_errors = 5;
    
    while (!SignalHandler::shouldStop()) {
        spdlog::debug("Requesting task...");
        auto taskJson = comm.fetchTask(cfg.uid, cfg.access_code);
        
        if (!taskJson.has_value()) {
            spdlog::error("Failed to fetch task (network error)");
            error_count++;
            
            if (error_count >= max_errors) {
                spdlog::error("Too many consecutive errors, increasing interval");
                std::this_thread::sleep_for(std::chrono::seconds(30));
            } else {
                // Ждём стандартный интервал
                for (int i = 0; i < cfg.poll_interval_sec; ++i) {
                    if (SignalHandler::shouldStop()) break;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            continue;
        }
        
        // Сброс счётчика ошибок при успешном ответе
        error_count = 0;
        
        // Парсим ответ
        auto response = TaskResponseParser::parse(*taskJson);
        
        // Обрабатываем в зависимости от кода
        switch(response.code) {
            case 1:  // Есть задание
                if (response.hasTask()) {
                    const auto& task = *response.task;
                    spdlog::info("🎯 Got task! code={}, session={}", 
                                task.task_code, task.session_id);
                    
                    // TODO: передать в TaskExecutor
                    spdlog::info("Task options: {}", task.options);
                    
                } else {
                    spdlog::error("Code 1 but no task data");
                }
                break;
                
            case 0:  // Нет задания
                spdlog::debug("No tasks, status: {}", response.status);
                break;
                
            case -2:  // Неверный access_code
                spdlog::warn("Invalid access code, clearing and exiting...");
                cfg.access_code = "";
                cfg.saveAccessCode("");
                // Завершаемся, чтобы systemd или другой менеджер перезапустил
                spdlog::info("Exiting to re-register on next start");
                return 0;
                break;
                
            case -3:  // Агент уже зарегистрирован (при регистрации)
                spdlog::warn("Agent already registered (this shouldn't happen in main loop)");
                break;
                
            default:  // Другие ошибки
                if (response.code < 0) {
                    spdlog::error("Server error {}: {}", 
                                response.code, response.message);
                    
                    if (response.code == -1) {
                        spdlog::error("Malformed response from server");
                    }
                }
                break;
        }
        
        // Ждём с проверкой на сигнал
        for (int i = 0; i < cfg.poll_interval_sec; ++i) {
            if (SignalHandler::shouldStop()) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    spdlog::info("Agent stopped gracefully");
    return 0;
}
