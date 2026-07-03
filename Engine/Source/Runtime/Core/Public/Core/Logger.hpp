#pragma once

#include "Core/Export.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>

namespace we::runtime::core {

class Logger {
public:
    enum class Level {
        Info,
        Warning,
        Error,
        Debug
    };

    struct LogEntry {
        Level level;
        std::string timestamp;
        std::string message;
        std::string formattedText;
    };

    CORE_API static void Init();
    CORE_API static void Shutdown();

    CORE_API static void Log(Level level, const std::string& message);
    CORE_API static void ReportError(const std::string& title, const std::string& description, bool fatal = false);

    // Retrieve and clear new logs for Console panel
    CORE_API static std::vector<LogEntry> GetNewLogs();

private:
    static std::string GetCurrentTimestamp();
    static std::string LevelToString(Level level);

    // Crash handling registration
    static void SetupCrashHandler();
#if defined(_WIN32)
    static long __stdcall EngineCrashHandler(struct _EXCEPTION_POINTERS* exceptionInfo);
#endif
    static void SignalHandler(int signal);

    static std::mutex s_Mutex;
    static std::ofstream s_LogFile;
    static std::vector<LogEntry> s_LogBuffer;
    static bool s_Initialized;
};

} // namespace we::runtime::core

namespace we {
    using Logger = we::runtime::core::Logger;
}

// Logging Macros
#define HE_INFO(msg)  ::we::Logger::Log(::we::Logger::Level::Info, msg)
#define HE_WARN(msg)  ::we::Logger::Log(::we::Logger::Level::Warning, msg)
#define HE_ERROR(msg) ::we::Logger::Log(::we::Logger::Level::Error, msg)
#define HE_DEBUG(msg) ::we::Logger::Log(::we::Logger::Level::Debug, msg)
