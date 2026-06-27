#include "Logger.hpp"
#include <SDL3/SDL_messagebox.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <csignal>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

namespace HouseEngine {

std::mutex Logger::s_Mutex;
std::ofstream Logger::s_LogFile;
std::vector<Logger::LogEntry> Logger::s_LogBuffer;
bool Logger::s_Initialized = false;

void Logger::Init() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    if (s_Initialized) return;

    s_LogFile.open("WindEffects.log", std::ios::out | std::ios::trunc);
    s_Initialized = true;

    // Log startup
    LogEntry entry{};
    entry.level = Level::Info;
    entry.timestamp = GetCurrentTimestamp();
    entry.message = "System Logger initialized. Writing to WindEffects.log.";
    entry.formattedText = "[" + entry.timestamp + "] [INFO] " + entry.message;
    s_LogBuffer.push_back(entry);

    std::cout << entry.formattedText << std::endl;
    if (s_LogFile.is_open()) {
        s_LogFile << entry.formattedText << std::endl;
    }

    // Set up crash filters
    SetupCrashHandler();
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    if (!s_Initialized) return;

    std::string fmtText = "[" + GetCurrentTimestamp() + "] [INFO] System Logger shutting down.";
    std::cout << fmtText << std::endl;
    if (s_LogFile.is_open()) {
        s_LogFile << fmtText << std::endl;
        s_LogFile.close();
    }
    s_Initialized = false;
}

void Logger::Log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    std::string timestamp = GetCurrentTimestamp();
    std::string lvlStr = LevelToString(level);
    std::string formatted = "[" + timestamp + "] [" + lvlStr + "] " + message;

    LogEntry entry{ level, timestamp, message, formatted };
    s_LogBuffer.push_back(entry);

    if (level == Level::Error) {
        std::cerr << formatted << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }

    if (s_Initialized && s_LogFile.is_open()) {
        s_LogFile << formatted << std::endl;
        s_LogFile.flush();
    }
}

void Logger::ReportError(const std::string& title, const std::string& description, bool fatal) {
    std::string logMsg = "Error Reported: " + title + " - " + description + (fatal ? " [FATAL]" : "");
    Log(Level::Error, logMsg);

    // Show native OS dialog popup box
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title.c_str(),
        description.c_str(),
        nullptr
    );

    if (fatal) {
        Shutdown();
        exit(1);
    }
}

std::vector<Logger::LogEntry> Logger::GetNewLogs() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::vector<LogEntry> logs = std::move(s_LogBuffer);
    s_LogBuffer.clear();
    return logs;
}

std::string Logger::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    struct tm buf;
#if defined(_WIN32)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&buf, &in_time_t);
#endif

    std::stringstream ss;
    ss << std::put_time(&buf, "%H:%M:%S");
    return ss.str();
}

std::string Logger::LevelToString(Level level) {
    switch (level) {
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
        case Level::Debug:   return "DEBUG";
    }
    return "UNKNOWN";
}

void Logger::SetupCrashHandler() {
    // 1. Register standard POSIX signals
    std::signal(SIGSEGV, SignalHandler);
    std::signal(SIGFPE, SignalHandler);
    std::signal(SIGILL, SignalHandler);
    std::signal(SIGABRT, SignalHandler);

    // 2. Register Windows SEH filter
#if defined(_WIN32)
    SetUnhandledExceptionFilter(EngineCrashHandler);
#endif
}

#if defined(_WIN32)
long __stdcall Logger::EngineCrashHandler(struct _EXCEPTION_POINTERS* exceptionInfo) {
    std::string exceptionName = "UNKNOWN EXCEPTION";
    DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;

    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:          exceptionName = "ACCESS VIOLATION (Null pointer dereference or invalid memory read/write)"; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:         exceptionName = "INTEGER DIVIDE BY ZERO"; break;
        case EXCEPTION_STACK_OVERFLOW:            exceptionName = "STACK OVERFLOW"; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:        exceptionName = "ILLEGAL INSTRUCTION"; break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:     exceptionName = "ARRAY BOUNDS EXCEEDED"; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:        exceptionName = "FLOATING POINT DIVIDE BY ZERO"; break;
        case EXCEPTION_PRIV_INSTRUCTION:          exceptionName = "PRIVILEGED INSTRUCTION VIOLATION"; break;
    }

    void* exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    std::string addressStr;
    std::stringstream ss;
    ss << "0x" << std::hex << exceptionAddress;
    addressStr = ss.str();

    std::string crashDetails = "Fatal Exception Intercepted:\n"
                               "Code: 0x" + std::to_string(code) + " (" + exceptionName + ")\n"
                               "Address: " + addressStr;

    Logger::Log(Level::Error, crashDetails);

    // Write crash Minidump file
    HANDLE hFile = CreateFileA(
        "WindEffects.dmp",
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    bool dumpSuccess = false;
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei{};
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;

        dumpSuccess = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            &mdei,
            nullptr,
            nullptr
        );
        CloseHandle(hFile);
    }

    std::string popupMsg = "A fatal engine crash occurred!\n\n" + crashDetails + "\n\n";
    if (dumpSuccess) {
        popupMsg += "A crash dump file was successfully saved to:\nWindEffects.dmp\n\n";
    }
    popupMsg += "Please review WindEffects.log for details.";

    // Show native error message box
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "WindEffects Engine - Fatal Crash",
        popupMsg.c_str(),
        nullptr
    );

    Shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void Logger::SignalHandler(int signal) {
    std::string sigName = "UNKNOWN SIGNAL";
    switch (signal) {
        case SIGSEGV: sigName = "SIGSEGV (Segmentation Fault / Access Violation)"; break;
        case SIGFPE:  sigName = "SIGFPE (Floating Point / Arithmetic Exception)"; break;
        case SIGILL:  sigName = "SIGILL (Illegal Instruction)"; break;
        case SIGABRT: sigName = "SIGABRT (Abort / Assertion Failure)"; break;
    }

    Logger::Log(Level::Error, "Fatal Signal Intercepted: " + sigName);

    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "WindEffects Engine - Fatal Crash",
        ("A fatal platform signal was intercepted:\n\n" + sigName + "\n\nEngine shutting down.").c_str(),
        nullptr
    );

    Shutdown();
    exit(1);
}

} // namespace HouseEngine
