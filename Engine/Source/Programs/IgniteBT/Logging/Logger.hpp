#pragma once

#include "Core/Types.hpp"
#include <sstream>
#include <mutex>
#include <fstream>
#include <iostream>

namespace IgniteBT {

// Log level
enum class LogLevel {
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// Log entry
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string category;
    Timestamp timestamp;
    std::string file;
    int line;
};

// Logger interface
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void Log(const LogEntry& entry) = 0;
};

// Console logger (colored output)
class ConsoleLogger : public ILogger {
public:
    void Log(const LogEntry& entry) override;
    
private:
    std::string GetLevelColor(LogLevel level);
    std::string GetLevelString(LogLevel level);
};

// File logger
class FileLogger : public ILogger {
public:
    explicit FileLogger(const Path& logFile);
    ~FileLogger() override;
    
    void Log(const LogEntry& entry) override;
    
private:
    std::ofstream m_file;
    std::mutex m_mutex;
};

// Composite logger (multiple loggers)
class CompositeLogger : public ILogger {
public:
    void AddLogger(std::shared_ptr<ILogger> logger);
    void Log(const LogEntry& entry) override;
    
private:
    std::vector<std::shared_ptr<ILogger>> m_loggers;
    std::mutex m_mutex;
};

// Main logger class
class Logger {
public:
    static Logger& Get();
    
    void SetLogLevel(LogLevel level);
    LogLevel GetLogLevel() const;
    
    void AddLogger(std::shared_ptr<ILogger> logger);
    void ClearLoggers();
    
    void Log(LogLevel level, const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    
    // Convenience methods
    void Verbose(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    void Debug(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    void Info(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    void Warning(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    void Error(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    void Fatal(const std::string& message, const std::string& category = "", const char* file = nullptr, int line = 0);
    
private:
    Logger();
    ~Logger() = default;
    
    LogLevel m_logLevel = LogLevel::Info;
    std::shared_ptr<CompositeLogger> m_compositeLogger;
    std::mutex m_mutex;
};

// Logging macros
#define LOG_VERBOSE(msg, cat) IgniteBT::Logger::Get().Verbose(msg, cat, __FILE__, __LINE__)
#define LOG_DEBUG(msg, cat) IgniteBT::Logger::Get().Debug(msg, cat, __FILE__, __LINE__)
#define LOG_INFO(msg, cat) IgniteBT::Logger::Get().Info(msg, cat, __FILE__, __LINE__)
#define LOG_WARNING(msg, cat) IgniteBT::Logger::Get().Warning(msg, cat, __FILE__, __LINE__)
#define LOG_ERROR(msg, cat) IgniteBT::Logger::Get().Error(msg, cat, __FILE__, __LINE__)
#define LOG_FATAL(msg, cat) IgniteBT::Logger::Get().Fatal(msg, cat, __FILE__, __LINE__)

} // namespace IgniteBT
