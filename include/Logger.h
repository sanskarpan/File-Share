#ifndef LOGGER_H
#define LOGGER_H

#include "Common.h"
#include <fstream>
#include <sstream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
private:
    static std::unique_ptr<Logger> instance;
    static std::mutex instance_mutex;
    
    std::ofstream log_file;
    LogLevel current_level;
    mutable std::mutex log_mutex;
    bool console_output;
    
    Logger();
    
    void writeLog(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level) const;
    std::string getCurrentTimestamp() const;

public:
    ~Logger();
    
    static Logger& getInstance();
    static void initialize(const std::string& filename = "p2p.log", LogLevel level = LogLevel::INFO);
    
    void setLevel(LogLevel level) { current_level = level; }
    void setConsoleOutput(bool enable) { console_output = enable; }
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    template<typename... Args>
    void debug(const std::string& format, Args... args);
    
    template<typename... Args>
    void info(const std::string& format, Args... args);
    
    template<typename... Args>
    void warning(const std::string& format, Args... args);
    
    template<typename... Args>
    void error(const std::string& format, Args... args);
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

template<typename... Args>
void Logger::debug(const std::string& format, Args... args) {
    if (current_level <= LogLevel::DEBUG) {
        std::ostringstream oss;
        // Simple format implementation - in production, use fmt library
        oss << format;
        writeLog(LogLevel::DEBUG, oss.str());
    }
}

template<typename... Args>
void Logger::info(const std::string& format, Args... args) {
    if (current_level <= LogLevel::INFO) {
        std::ostringstream oss;
        oss << format;
        writeLog(LogLevel::INFO, oss.str());
    }
}

template<typename... Args>
void Logger::warning(const std::string& format, Args... args) {
    if (current_level <= LogLevel::WARNING) {
        std::ostringstream oss;
        oss << format;
        writeLog(LogLevel::WARNING, oss.str());
    }
}

template<typename... Args>
void Logger::error(const std::string& format, Args... args) {
    if (current_level <= LogLevel::ERROR) {
        std::ostringstream oss;
        oss << format;
        writeLog(LogLevel::ERROR, oss.str());
    }
}

#endif