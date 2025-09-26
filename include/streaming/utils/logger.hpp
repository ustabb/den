// include/streaming/utils/logger.hpp
#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <queue>
#include <atomic>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace streaming {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
};

class Logger {
public:
    struct LogConfig {
        std::string log_dir = "./logs";
        std::string log_file = "streaming_engine.log";
        LogLevel level = LogLevel::INFO;
        uint32_t max_file_size = 10 * 1024 * 1024; // 10MB
        uint32_t max_files = 5;
        bool console_output = true;
        bool file_output = true;
        bool async_logging = true;
        uint32_t flush_interval_sec = 5;
    };

    static Logger& get_instance();
    
    bool initialize(const LogConfig& config);
    void shutdown();
    
    // Logging methods
    void trace(const std::string& message, const std::string& module = "");
    void debug(const std::string& message, const std::string& module = "");
    void info(const std::string& message, const std::string& module = "");
    void warn(const std::string& message, const std::string& module = "");
    void error(const std::string& message, const std::string& module = "");
    void critical(const std::string& message, const std::string& module = "");

    // Structured logging
    void log_with_fields(LogLevel level, const std::string& message, 
                        const std::unordered_map<std::string, std::string>& fields);
    
    // Performance logging
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& operation, LogLevel level = LogLevel::DEBUG);
        ~ScopedTimer();
        
    private:
        std::string operation_;
        LogLevel level_;
        std::chrono::steady_clock::time_point start_time_;
    };

    // Metrics logging
    void log_metric(const std::string& name, double value, 
                   const std::unordered_map<std::string, std::string>& tags = {});

private:
    Logger() = default;
    ~Logger();
    
    void rotate_log_if_needed();
    void background_flush();
    
    LogConfig config_;
    std::shared_ptr<spdlog::logger> logger_;
    std::atomic<bool> initialized_{false};
    std::thread flush_thread_;
};

// Convenience macros
#define LOG_TRACE(msg) streaming::Logger::get_instance().trace(msg, MODULE_NAME)
#define LOG_DEBUG(msg) streaming::Logger::get_instance().debug(msg, MODULE_NAME)
#define LOG_INFO(msg) streaming::Logger::get_instance().info(msg, MODULE_NAME)
#define LOG_WARN(msg) streaming::Logger::get_instance().warn(msg, MODULE_NAME)
#define LOG_ERROR(msg) streaming::Logger::get_instance().error(msg, MODULE_NAME)
#define LOG_CRITICAL(msg) streaming::Logger::get_instance().critical(msg, MODULE_NAME)

#define LOG_TIMER(op) streaming::Logger::ScopedTimer timer_##__LINE__(op)

} // namespace streaming