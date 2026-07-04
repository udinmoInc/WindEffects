#pragma once

#include "Core/Export.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace we::runtime::core {

class Logger {
public:
    enum class Level {
        Trace = 0,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    struct LogRecord {
        Level level = Level::Info;
        std::string category;
        std::string message;
        std::string timestamp;
        uint64_t frameNumber = 0;
        uint32_t threadId = 0;
        std::string file;
        int line = 0;
        std::string function;
        std::string formattedText;
        std::string stackTrace;
    };

    using LogListener = std::function<void(const LogRecord&)>;

    CORE_API static void Init();
    CORE_API static void Shutdown();

    CORE_API static void Log(
        Level level,
        std::string_view category,
        const std::string& message,
        const char* file = nullptr,
        int line = 0,
        const char* function = nullptr);

    // Backward-compatible overload for legacy call sites.
    CORE_API static void Log(Level level, const std::string& message);

    CORE_API static void ReportError(const std::string& title, const std::string& description, bool fatal = false);

    // UI / diagnostics
    CORE_API static std::vector<LogRecord> GetNewLogs();
    CORE_API static std::vector<LogRecord> GetHistory();
    CORE_API static void AddListener(LogListener listener);
    CORE_API static void ClearHistory();

    CORE_API static void SetMinimumLevel(Level level);
    CORE_API static Level GetMinimumLevel();
    CORE_API static const std::string& GetActiveLogFilePath();

    CORE_API static void Flush();

private:
    static std::string GetCurrentTimestamp();
    static std::string LevelToString(Level level);
    static std::string FormatRecord(const LogRecord& record);
    static void EnqueueRecord(LogRecord record, bool flushImmediately);
    static void WriterThreadMain();
    static void WriteRecordToOutputs(const LogRecord& record);
    static void RotateLogFilesIfNeeded();
    static void SetupCrashHandler();
#if defined(_WIN32)
    static long __stdcall EngineCrashHandler(struct _EXCEPTION_POINTERS* exceptionInfo);
#endif
    static void SignalHandler(int signal);

    static std::mutex s_Mutex;
    static std::condition_variable s_Cv;
    static std::deque<LogRecord> s_PendingRecords;
    static std::vector<LogRecord> s_History;
    static std::vector<LogRecord> s_NewLogs;
    static std::vector<LogListener> s_Listeners;
    static std::thread s_WriterThread;
    static std::atomic<bool> s_Running;
    static std::atomic<bool> s_Initialized;
    static std::atomic<Level> s_MinimumLevel;
    static std::ofstream s_LogFile;
    static std::string s_LogFilePath;
    static std::string s_SessionDirectory;
    static constexpr size_t kMaxHistoryEntries = 10000;
    static constexpr size_t kMaxLogFileBytes = 16 * 1024 * 1024;
};

} // namespace we::runtime::core

namespace we {
    using Logger = we::runtime::core::Logger;
}

#include "Core/DiagnosticMacros.hpp"
