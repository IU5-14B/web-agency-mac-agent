#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "config/Config.h"
#include "communicator/Communicator.h"
#include "executor/TaskExecutor.h"
#include "utils/SignalHandler.h"
#include "utils/TaskResponse.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    
    // Resolve config path early to place log next to project root
    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    } else {
        if (fs::exists("config/config.json")) {
            config_path = "config/config.json";
        } else if (fs::exists("../config/config.json")) {
            config_path = "../config/config.json";
        }
    }

    // Place agent.log in project root (sibling of config/)
    fs::path log_path = "agent.log";
    try {
        fs::path cfg_abs = fs::weakly_canonical(config_path);
        fs::path root_dir = cfg_abs.parent_path().parent_path();
        if (!root_dir.empty()) {
            log_path = root_dir / "agent.log";
        }
    } catch (...) {
        // fall back to default log_path
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true);
    file_sink->set_level(spdlog::level::debug);
    
    spdlog::logger logger("agent", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    logger.flush_on(spdlog::level::info);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
    spdlog::flush_every(std::chrono::seconds(1));
    
    spdlog::info("Web Agent starting...");

    
    Config cfg;
    if (!cfg.loadFromFile(config_path)) {
        spdlog::error("❌ Failed to load config. Exiting.");
        return 1;
    }

    spdlog::info("✅ Config loaded: UID={}, server={}, interval={} sec", 
                 cfg.uid, cfg.server_url, cfg.poll_interval_sec);

    // Инициализация обработчика сигналов
    SignalHandler::init([]() {
        spdlog::info("Shutdown requested, cleaning up...");
    });

    
    Communicator comm(cfg.server_url, false);
    TaskExecutor executor(cfg.work_dir, cfg.results_dir, &cfg);
    
    executor.setTimeout(std::chrono::seconds(60));

    
    if (cfg.access_code.empty()) {
        spdlog::info("No access code, registering...");
        auto code = comm.registerAgent(cfg.uid);
        if (code.has_value()) {
            cfg.saveAccessCode(*code);
            spdlog::info("✅ Access code saved: {}", *code);
        } else {
            spdlog::error("❌ Registration failed. Exiting.");
            return 1;
        }
    } else {
        spdlog::info("Using existing access code: {}", cfg.access_code);
    }

    // Основной цикл
    spdlog::info("Entering main loop. Press Ctrl+C to stop.");
    
    int error_count = 0;
    const int max_errors = 5;
    
    while (!SignalHandler::shouldStop()) {
        
        spdlog::debug("Requesting task from server...");
        auto taskJson = comm.fetchTask(cfg.uid, cfg.access_code);
        
        if (!taskJson.has_value()) {
            error_count++;
            spdlog::error("Failed to fetch task (network error) - {}/{}", 
                         error_count, max_errors);
            
            if (error_count >= max_errors) {
                int wait = std::min(cfg.poll_interval_sec * 2, 300);
                spdlog::warn("Too many errors, waiting {}s", wait);
                for (int i = 0; i < wait; ++i) {
                    if (SignalHandler::shouldStop()) break;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            } else {
                
                for (int i = 0; i < cfg.poll_interval_sec; ++i) {
                    if (SignalHandler::shouldStop()) break;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            continue;
        }
        
        error_count = 0;
        
        
        auto response = TaskResponseParser::parse(*taskJson);
        
        if (response.code == 1 && response.hasTask()) {
            
            const auto& task_info = *response.task;
            spdlog::info("Got task! code={}, session={}", 
                        task_info.task_code, task_info.session_id);
            
            
            Task task;
            task.task_code = task_info.task_code;
            task.options = task_info.options;
            task.session_id = task_info.session_id;
            task.status = task_info.status;
            
            
            std::string exec_message;
            std::vector<std::string> result_files;
            int exec_result = executor.execute(task, exec_message, result_files);
            
            spdlog::info("Execution result: {} - {}", exec_result, exec_message);
            
            
            bool sent = comm.sendResult(
                cfg.uid,
                cfg.access_code,
                task.session_id,
                exec_result,
                exec_message,
                result_files.size(),
                result_files
            );
            
            if (sent) {
                spdlog::info("✅ Result sent successfully for session: {}", task.session_id);
                // Keep result files so user can inspect them locally after upload
                for (const auto& f : result_files) {
                    spdlog::info("Result file retained at {}", f);
                }

            } else {
                spdlog::error("❌ Failed to send result for session: {}", task.session_id);
            }
            
        } else if (response.code == 0) {
            
            spdlog::debug("No tasks, status: {}", response.status);
            
        } else if (response.code == -2 || response.code == -13) {
            
            spdlog::warn("Invalid access code ({}), clearing and re-registering...", response.code);
            cfg.access_code = "";
            cfg.saveAccessCode("");
            
            
        } else if (response.code < 0) {
            
            spdlog::error("Server error {}: {}", response.code, response.message);
        }
        
        for (int i = 0; i < cfg.poll_interval_sec; ++i) {
            if (SignalHandler::shouldStop()) break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    spdlog::info("Agent stopped gracefully");
    return 0;
}
