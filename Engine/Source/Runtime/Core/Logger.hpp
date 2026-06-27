#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>

namespace HouseEngine {

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

    static void Init();
    static void Shutdown();

    static void Log(Level level, const std::string& message);
    static void ReportError(const std::string& title, const std::string& description, bool fatal = false);

    // Retrieve and clear new logs for Console panel
    static std::vector<LogEntry> GetNewLogs();

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

} // namespace HouseEngine

// Logging Macros
#define HE_INFO(msg)  ::HouseEngine::Logger::Log(::HouseEngine::Logger::Level::Info, msg)
#define HE_WARN(msg)  ::HouseEngine::Logger::Log(::HouseEngine::Logger::Level::Warning, msg)
#define HE_ERROR(msg) ::HouseEngine::Logger::Log(::HouseEngine::Logger::Level::Error, msg)
#define HE_DEBUG(msg) ::HouseEngine::Logger::Log(::HouseEngine::Logger::Level::Debug, msg)
