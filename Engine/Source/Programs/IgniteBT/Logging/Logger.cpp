#include "Logger.hpp"
#include <iomanip>
#include <ctime>

namespace IgniteBT {

// Console Logger
void ConsoleLogger::Log(const LogEntry& entry) {
    std::string color = GetLevelColor(entry.level);
    std::string levelStr = GetLevelString(entry.level);
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::cout << color
              << "[" << std::put_time(&tm, "%H:%M:%S") << "] "
              << "[" << levelStr << "]";
    
    if (!entry.category.empty()) {
        std::cout << " [" << entry.category << "]";
    }
    
    std::cout << ": " << entry.message << "\033[0m" << std::endl;
}

std::string ConsoleLogger::GetLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Verbose: return "\033[90m";    // Gray
        case LogLevel::Debug:   return "\033[36m";    // Cyan
        case LogLevel::Info:    return "\033[32m";    // Green
        case LogLevel::Warning: return "\033[33m";    // Yellow
        case LogLevel::Error:   return "\033[31m";    // Red
        case LogLevel::Fatal:   return "\033[35m";    // Magenta
        default:                return "\033[0m";
    }
}

std::string ConsoleLogger::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Verbose: return "VERB";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "?????";
    }
}

// File Logger
FileLogger::FileLogger(const Path& logFile) {
    m_file.open(logFile, std::ios::out | std::ios::app);
}

FileLogger::~FileLogger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void FileLogger::Log(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_file.is_open()) return;
    
    auto time_t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    m_file << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
           << "[" << static_cast<int>(entry.level) << "]";
    
    if (!entry.category.empty()) {
        m_file << " [" << entry.category << "]";
    }
    
    m_file << ": " << entry.message << std::endl;
    m_file.flush();
}

// Composite Logger
void CompositeLogger::AddLogger(std::shared_ptr<ILogger> logger) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loggers.push_back(logger);
}

void CompositeLogger::Log(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& logger : m_loggers) {
        logger->Log(entry);
    }
}

// Main Logger
Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    m_compositeLogger = std::make_shared<CompositeLogger>();
    m_compositeLogger->AddLogger(std::make_shared<ConsoleLogger>());
}

void Logger::SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logLevel = level;
}

LogLevel Logger::GetLogLevel() const {
    return m_logLevel;
}

void Logger::AddLogger(std::shared_ptr<ILogger> logger) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_compositeLogger->AddLogger(logger);
}

void Logger::ClearLoggers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_compositeLogger = std::make_shared<CompositeLogger>();
}

void Logger::Log(LogLevel level, const std::string& message, const std::string& category, const char* file, int line) {
    if (level < m_logLevel) return;
    
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.category = category;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.file = file ? file : "";
    entry.line = line;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_compositeLogger->Log(entry);
}

void Logger::Verbose(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Verbose, message, category, file, line);
}

void Logger::Debug(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Debug, message, category, file, line);
}

void Logger::Info(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Info, message, category, file, line);
}

void Logger::Warning(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Warning, message, category, file, line);
}

void Logger::Error(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Error, message, category, file, line);
}

void Logger::Fatal(const std::string& message, const std::string& category, const char* file, int line) {
    Log(LogLevel::Fatal, message, category, file, line);
}

} // namespace IgniteBT
