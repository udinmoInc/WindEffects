#include "Core/Logger.hpp"
#include "Core/FrameCounter.hpp"
#include "Core/LogCategory.hpp"

#if WE_HAS_SDL3
#include <SDL3/SDL_messagebox.h>
#endif

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#include <chrono>
#include <csignal>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#endif

namespace we::runtime::core {

std::mutex Logger::s_Mutex;
std::condition_variable Logger::s_Cv;
std::deque<Logger::LogRecord> Logger::s_PendingRecords;
std::vector<Logger::LogRecord> Logger::s_History;
std::vector<Logger::LogRecord> Logger::s_NewLogs;
std::vector<Logger::LogListener> Logger::s_Listeners;
std::thread Logger::s_WriterThread;
std::atomic<bool> Logger::s_Running{ false };
std::atomic<bool> Logger::s_Initialized{ false };
std::atomic<Logger::Level> Logger::s_MinimumLevel{ Logger::Level::Trace };
std::ofstream Logger::s_LogFile;
std::string Logger::s_LogFilePath;
std::string Logger::s_SessionDirectory;

void Logger::Init() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    if (s_Initialized.load()) return;

    const auto now = std::chrono::system_clock::now();
    const auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    s_SessionDirectory = "Saved/Logs/Sessions/Session_" + std::to_string(epoch);
    std::error_code ec;
    std::filesystem::create_directories(s_SessionDirectory, ec);
    std::filesystem::create_directories("Saved/Logs", ec);
    std::filesystem::create_directories("Saved/Crashes", ec);

    s_LogFilePath = s_SessionDirectory + "/WindEffects.log";
    s_LogFile.open(s_LogFilePath, std::ios::out | std::ios::trunc);
    if (!s_LogFile.is_open()) {
        s_LogFilePath = "Saved/Logs/WindEffects.log";
        s_LogFile.open(s_LogFilePath, std::ios::out | std::ios::trunc);
    }

    s_Running.store(true);
    s_WriterThread = std::thread(WriterThreadMain);
    s_Initialized.store(true);

    LogRecord startup{};
    startup.level = Level::Info;
    startup.category = "Startup";
    startup.message = "WindEffects logger initialized. Session log: " + s_LogFilePath;
    startup.timestamp = GetCurrentTimestamp();
    startup.frameNumber = FrameCounter::GetFrameNumber();
#if defined(_WIN32)
    startup.threadId = static_cast<uint32_t>(GetCurrentThreadId());
#endif
    startup.formattedText = FormatRecord(startup);
    EnqueueRecord(std::move(startup), true);

    SetupCrashHandler();
}

void Logger::Shutdown() {
    if (!s_Initialized.load()) return;

    {
        LogRecord shutdownRecord{};
        shutdownRecord.level = Level::Info;
        shutdownRecord.category = "Startup";
        shutdownRecord.message = "WindEffects logger shutting down.";
        shutdownRecord.timestamp = GetCurrentTimestamp();
        shutdownRecord.frameNumber = FrameCounter::GetFrameNumber();
        shutdownRecord.formattedText = FormatRecord(shutdownRecord);
        EnqueueRecord(std::move(shutdownRecord), true);
    }

    s_Running.store(false);
    s_Cv.notify_all();
    if (s_WriterThread.joinable()) {
        s_WriterThread.join();
    }

    std::lock_guard<std::mutex> lock(s_Mutex);
    if (s_LogFile.is_open()) {
        s_LogFile.close();
    }
    s_Initialized.store(false);
}

void Logger::Log(Level level, const std::string& message) {
    Log(level, LogCategory::General, message);
}

void Logger::Log(Level level, std::string_view category, const std::string& message, const char* file, int line, const char* function) {
    if (static_cast<int>(level) < static_cast<int>(s_MinimumLevel.load())) {
        return;
    }

    LogRecord record{};
    record.level = level;
    record.category = std::string(category);
    record.message = message;
    record.timestamp = GetCurrentTimestamp();
    record.frameNumber = FrameCounter::GetFrameNumber();
#if defined(_WIN32)
    record.threadId = static_cast<uint32_t>(GetCurrentThreadId());
#endif
    if (file) record.file = file;
    record.line = line;
    if (function) record.function = function;
    record.formattedText = FormatRecord(record);

    const bool flushImmediately = (level >= Level::Error);
    EnqueueRecord(std::move(record), flushImmediately);
}

void Logger::EnqueueRecord(LogRecord record, bool flushImmediately) {
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_PendingRecords.push_back(std::move(record));
    }
    s_Cv.notify_one();

    if (flushImmediately) {
        Flush();
    }
}

void Logger::WriterThreadMain() {
    while (s_Running.load() || !s_PendingRecords.empty()) {
        std::deque<LogRecord> batch;
        {
            std::unique_lock<std::mutex> lock(s_Mutex);
            s_Cv.wait_for(lock, std::chrono::milliseconds(50), [] {
                return !s_PendingRecords.empty() || !s_Running.load();
            });
            batch.swap(s_PendingRecords);
        }

        for (const LogRecord& record : batch) {
            WriteRecordToOutputs(record);
        }
    }
}

void Logger::WriteRecordToOutputs(const LogRecord& record) {
    std::vector<LogListener> listenersCopy;
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (s_History.size() >= kMaxHistoryEntries) {
            s_History.erase(s_History.begin(), s_History.begin() + (kMaxHistoryEntries / 4));
        }
        s_History.push_back(record);
        s_NewLogs.push_back(record);
        listenersCopy = s_Listeners;

        RotateLogFilesIfNeeded();
        if (s_LogFile.is_open()) {
            s_LogFile << record.formattedText << std::endl;
        }
    }

    if (record.level >= Level::Error) {
        std::cerr << record.formattedText << std::endl;
    } else if (record.level >= Level::Warning) {
        std::cout << record.formattedText << std::endl;
    } else {
        std::cout << record.formattedText << std::endl;
    }

    for (const auto& listener : listenersCopy) {
        if (listener) listener(record);
    }
}

void Logger::RotateLogFilesIfNeeded() {
    if (!s_LogFile.is_open()) return;
    s_LogFile.flush();
    std::error_code ec;
    const auto size = std::filesystem::file_size(s_LogFilePath, ec);
    if (ec || size < kMaxLogFileBytes) return;

    s_LogFile.close();
    const std::string rotated = s_SessionDirectory + "/WindEffects_" + std::to_string(size) + ".log";
    std::filesystem::rename(s_LogFilePath, rotated, ec);
    s_LogFile.open(s_LogFilePath, std::ios::out | std::ios::app);
}

void Logger::ReportError(const std::string& title, const std::string& description, bool fatal) {
    Log(Level::Critical, "General", "Error Reported: " + title + " - " + description + (fatal ? " [FATAL]" : ""));

#if WE_HAS_SDL3
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), description.c_str(), nullptr);
#endif

    if (fatal) {
        Shutdown();
        std::exit(1);
    }
}

std::vector<Logger::LogRecord> Logger::GetNewLogs() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::vector<LogRecord> logs = std::move(s_NewLogs);
    s_NewLogs.clear();
    return logs;
}

std::vector<Logger::LogRecord> Logger::GetHistory() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_History;
}

void Logger::AddListener(LogListener listener) {
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Listeners.push_back(std::move(listener));
}

void Logger::ClearHistory() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_History.clear();
    s_NewLogs.clear();
}

void Logger::SetMinimumLevel(Level level) {
    s_MinimumLevel.store(level);
}

Logger::Level Logger::GetMinimumLevel() {
    return s_MinimumLevel.load();
}

const std::string& Logger::GetActiveLogFilePath() {
    return s_LogFilePath;
}

void Logger::Flush() {
    std::unique_lock<std::mutex> lock(s_Mutex);
    s_Cv.wait_for(lock, std::chrono::milliseconds(250), [] { return s_PendingRecords.empty(); });
    if (s_LogFile.is_open()) {
        s_LogFile.flush();
    }
}

std::string Logger::GetCurrentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);

    struct tm buf{};
#if defined(_WIN32)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif

    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::LevelToString(Level level) {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error: return "ERROR";
        case Level::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

std::string Logger::FormatRecord(const LogRecord& record) {
    std::ostringstream ss;
    ss << '[' << record.timestamp << ']'
       << " [Frame:" << record.frameNumber << ']'
       << " [T:" << record.threadId << ']'
       << " [" << LevelToString(record.level) << ']'
       << " [" << record.category << ']';
    if (!record.file.empty()) {
        ss << " (" << record.file << ':' << record.line;
        if (!record.function.empty()) ss << " " << record.function;
        ss << ')';
    }
    ss << ' ' << record.message;
    return ss.str();
}

void Logger::SetupCrashHandler() {
    std::signal(SIGFPE, SignalHandler);
    std::signal(SIGILL, SignalHandler);
    std::signal(SIGABRT, SignalHandler);
#if defined(_WIN32)
    SetUnhandledExceptionFilter(EngineCrashHandler);
#endif
}

#if defined(_WIN32)
long __stdcall Logger::EngineCrashHandler(struct _EXCEPTION_POINTERS* exceptionInfo) {
    std::string exceptionName = "UNKNOWN EXCEPTION";
    const DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: exceptionName = "ACCESS VIOLATION"; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: exceptionName = "INTEGER DIVIDE BY ZERO"; break;
        case EXCEPTION_STACK_OVERFLOW: exceptionName = "STACK OVERFLOW"; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION: exceptionName = "ILLEGAL INSTRUCTION"; break;
        default: break;
    }

    std::ostringstream addressStream;
    addressStream << "0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress;
    Log(Level::Critical, "Crash", "Fatal exception: " + exceptionName + " at " + addressStream.str());

    const std::string crashDir = "Saved/Crashes/Latest";
    std::error_code ec;
    if (std::filesystem::exists(crashDir)) {
        const std::string backupDir = "Saved/Crashes/Crash_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::filesystem::rename(crashDir, backupDir, ec);
    }
    std::filesystem::create_directories(crashDir, ec);

    HANDLE hFile = CreateFileA((crashDir + "/WindEffects.dmp").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei{};
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, nullptr, nullptr);
        CloseHandle(hFile);
    }

#if WE_HAS_NLOHMANN_JSON
    nlohmann::json crashJson;
    crashJson["CrashTime"] = GetCurrentTimestamp();
    crashJson["CrashType"] = "Unhandled Exception";
    crashJson["Project"] = "WindEffects";
    crashJson["EngineVersion"] = "1.0.0";
    crashJson["ExceptionName"] = exceptionName;
    crashJson["LogFile"] = s_LogFilePath;
    std::ofstream(crashDir + "/Crash.json") << crashJson.dump(4);
#endif

    std::ofstream stackFile(crashDir + "/StackTrace.txt");
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymInitialize(process, NULL, TRUE);

    STACKFRAME64 stackFrame{};
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = exceptionInfo->ContextRecord->Rip;
    stackFrame.AddrFrame.Offset = exceptionInfo->ContextRecord->Rbp;
    stackFrame.AddrStack.Offset = exceptionInfo->ContextRecord->Rsp;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    for (int frameNum = 0; frameNum < 64; ++frameNum) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stackFrame, exceptionInfo->ContextRecord, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
        if (stackFrame.AddrPC.Offset == 0) break;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, pSymbol)) {
            stackFile << pSymbol->Name << " - 0x" << std::hex << stackFrame.AddrPC.Offset << std::endl;
        } else {
            stackFile << "Unknown Function - 0x" << std::hex << stackFrame.AddrPC.Offset << std::endl;
        }
    }
    SymCleanup(process);
    stackFile.close();

    Flush();
    if (!s_LogFilePath.empty()) {
        std::filesystem::copy_file(s_LogFilePath, crashDir + "/Engine.log", std::filesystem::copy_options::overwrite_existing, ec);
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string reporterPath = "WECrashReporter.exe";
    CreateProcessA(nullptr, reporterPath.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi);
    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    Shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void Logger::SignalHandler(int signal) {
    std::string sigName = "UNKNOWN SIGNAL";
    switch (signal) {
        case SIGSEGV: sigName = "SIGSEGV"; break;
        case SIGFPE: sigName = "SIGFPE"; break;
        case SIGILL: sigName = "SIGILL"; break;
        case SIGABRT: sigName = "SIGABRT"; break;
        default: break;
    }
    Log(Level::Critical, "Crash", "Fatal signal intercepted: " + sigName);
    Shutdown();
    std::exit(1);
}

} // namespace we::runtime::core
