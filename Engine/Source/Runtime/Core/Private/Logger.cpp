#include "Core/Logger.hpp"

#if WE_HAS_SDL3
#include <SDL3/SDL_messagebox.h>
#endif

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

#include <iostream>
#include <iomanip>
#include <chrono>
#include <csignal>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#endif


namespace we::runtime::core {

std::mutex Logger::s_Mutex;
std::ofstream Logger::s_LogFile;
std::vector<Logger::LogEntry> Logger::s_LogBuffer;
bool Logger::s_Initialized = false;

void Logger::Init() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    if (s_Initialized) return;

    std::error_code ec;
    std::filesystem::create_directories("logs", ec);
    s_LogFile.open("logs/WindEffects.log", std::ios::out | std::ios::trunc);

    if (!s_LogFile.is_open()) {
        s_LogFile.open("WindEffects.log", std::ios::out | std::ios::trunc);
    }
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
#if WE_HAS_SDL3
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title.c_str(),
        description.c_str(),
        nullptr
    );
#endif

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
    // std::signal(SIGSEGV, SignalHandler);
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
        case EXCEPTION_INT_DIVIDE_BY_ZERO:        exceptionName = "INTEGER DIVIDE BY ZERO"; break;
        case EXCEPTION_STACK_OVERFLOW:            exceptionName = "STACK OVERFLOW"; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:       exceptionName = "ILLEGAL INSTRUCTION"; break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:     exceptionName = "ARRAY BOUNDS EXCEEDED"; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:        exceptionName = "FLOATING POINT DIVIDE BY ZERO"; break;
        case EXCEPTION_PRIV_INSTRUCTION:          exceptionName = "PRIVILEGED INSTRUCTION VIOLATION"; break;
    }

    void* exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << code;
    const std::string codeStr = ss.str();
    ss.str({});
    ss.clear();
    ss << "0x" << exceptionAddress;
    const std::string addressStr = ss.str();

    std::string crashDetails = "Fatal Exception Intercepted:\n"
                               "Code: " + codeStr + " (" + exceptionName + ")\n"
                               "Address: " + addressStr;

    Logger::Log(Level::Error, crashDetails);

    // Ensure Crash directories exist
    std::string crashDir = "Saved/Logs/Crashes/Latest";
    if (std::filesystem::exists(crashDir)) {
        std::string backupDir = "Saved/Logs/Crashes/Crash_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::error_code ec;
        std::filesystem::rename(crashDir, backupDir, ec);
    }
    std::filesystem::create_directories(crashDir);

    // 1. Minidump
    HANDLE hFile = CreateFileA(
        (crashDir + "/WindEffects.dmp").c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei{};
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionInfo;
        mdei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, nullptr, nullptr);
        CloseHandle(hFile);
    }

    // 2. Exception.json
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json exJson;
    exJson["ExceptionCode"] = code;
    exJson["ExceptionName"] = exceptionName;
    exJson["Address"] = addressStr;
    std::ofstream exFile(crashDir + "/Exception.json");
    exFile << exJson.dump(4);
    exFile.close();
#endif

    // 3. Crash.json
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json crashJson;
    crashJson["CrashTime"] = GetCurrentTimestamp();
    crashJson["CrashType"] = "Unhandled Exception";
    crashJson["Project"] = "WindEffects";
    crashJson["EngineVersion"] = "1.0.0";
    crashJson["Thread"] = std::to_string(GetCurrentThreadId());
    std::ofstream cFile(crashDir + "/Crash.json");
    cFile << crashJson.dump(4);
    cFile.close();
#endif

    // 4. System.json
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json sysJson;
    sysJson["WindowsVersion"] = "Windows";
    std::ofstream sysFile(crashDir + "/System.json");
    sysFile << sysJson.dump(4);
    sysFile.close();
#endif

    // 5. Memory.json
#if WE_HAS_NLOHMANN_JSON
    PROCESS_MEMORY_COUNTERS pmc;
    nlohmann::json memJson;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memJson["WorkingSetSize"] = pmc.WorkingSetSize;
        memJson["PagefileUsage"] = pmc.PagefileUsage;
    }
    std::ofstream memFile(crashDir + "/Memory.json");
    memFile << memJson.dump(4);
    memFile.close();
#endif

    // 6. Modules.json
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json modJson = nlohmann::json::array();
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char szModName[MAX_PATH];
            if (GetModuleFileNameExA(GetCurrentProcess(), hMods[i], szModName, sizeof(szModName) / sizeof(char))) {
                nlohmann::json mod;
                mod["Path"] = szModName;
                mod["BaseAddress"] = (uint64_t)hMods[i];
                modJson.push_back(mod);
            }
        }
    }
    std::ofstream modFile(crashDir + "/Modules.json");
    modFile << modJson.dump(4);
    modFile.close();
#endif

    // 7. StackTrace.txt
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
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stackFrame, exceptionInfo->ContextRecord, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }
        if (stackFrame.AddrPC.Offset == 0) {
            break;
        }
        DWORD64 displacement = 0;
        if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, pSymbol)) {
            stackFile << pSymbol->Name << " - 0x" << std::hex << stackFrame.AddrPC.Offset << std::endl;
        } else {
            stackFile << "Unknown Function - 0x" << std::hex << stackFrame.AddrPC.Offset << std::endl;
        }
    }
    SymCleanup(process);
    stackFile.close();

    // Copy Engine.log
    if (s_LogFile.is_open()) {
        s_LogFile.close();
    }
    std::error_code ec_copy;
    std::filesystem::copy_file("logs/WindEffects.log", crashDir + "/Engine.log", std::filesystem::copy_options::overwrite_existing, ec_copy);

    // Check for crash loop - prevent launching crash reporter if too many recent crashes
    static int crashCount = 0;
    static auto lastCrashTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastCrash = std::chrono::duration_cast<std::chrono::seconds>(now - lastCrashTime).count();
    
    // Reset counter if it's been more than 60 seconds since last crash
    if (timeSinceLastCrash > 60) {
        crashCount = 0;
    }
    
    crashCount++;
    lastCrashTime = now;
    
    // Don't launch crash reporter if we've crashed 3+ times in 60 seconds
    if (crashCount >= 3) {
        Logger::Log(Level::Error, "Crash loop detected - skipping crash reporter launch to prevent infinite loop");
        Shutdown();
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // Launch WeCrashReporter.exe
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string reporterPath = "WECrashReporter.exe";
    CreateProcessA(nullptr, (LPSTR)reporterPath.c_str(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi);
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
        case SIGSEGV: sigName = "SIGSEGV (Segmentation Fault / Access Violation)"; break;
        case SIGFPE:  sigName = "SIGFPE (Floating Point / Arithmetic Exception)"; break;
        case SIGILL:  sigName = "SIGILL (Illegal Instruction)"; break;
        case SIGABRT: sigName = "SIGABRT (Abort / Assertion Failure)"; break;
    }

    Logger::Log(Level::Error, "Fatal Signal Intercepted: " + sigName);

    // Ensure Crash directories exist
    std::string crashDir = "Saved/Logs/Crashes/Latest";
    if (std::filesystem::exists(crashDir)) {
        std::string backupDir = "Saved/Logs/Crashes/Crash_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::error_code ec;
        std::filesystem::rename(crashDir, backupDir, ec);
    }
    std::filesystem::create_directories(crashDir);

#if WE_HAS_NLOHMANN_JSON
    nlohmann::json crashJson;
    crashJson["CrashTime"] = GetCurrentTimestamp();
    crashJson["CrashType"] = "Fatal Signal";
    crashJson["Project"] = "WindEffects";
    crashJson["EngineVersion"] = "1.0.0";
    
    std::ofstream cFile(crashDir + "/Crash.json");
    cFile << crashJson.dump(4);
    cFile.close();
#endif

#if WE_HAS_NLOHMANN_JSON
    nlohmann::json exJson;
    exJson["ExceptionName"] = sigName;
    std::ofstream exFile(crashDir + "/Exception.json");
    exFile << exJson.dump(4);
    exFile.close();
#endif

    if (s_LogFile.is_open()) {
        s_LogFile.close();
    }
    std::error_code ec_copy;
    std::filesystem::copy_file("logs/WindEffects.log", crashDir + "/Engine.log", std::filesystem::copy_options::overwrite_existing, ec_copy);

#if defined(_WIN32)
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string reporterPath = "WECrashReporter.exe";
    CreateProcessA(nullptr, (LPSTR)reporterPath.c_str(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &si, &pi);
    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif

    Shutdown();
    exit(1);
}
} // namespace we::runtime::core
