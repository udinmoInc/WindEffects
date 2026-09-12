// ==============================================================================
// WindEffects — Core — EngineWatchdog
// Automated heartbeat monitoring and main-thread stall/freeze detection.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/EngineWatchdog.h"
#include "Core/Logger.h"
#include "Core/LogCategory.h"
#include "Core/Paths.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

namespace we::runtime::core {

namespace {

int64_t GetCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

}

EngineWatchdog& EngineWatchdog::Get() {
    static EngineWatchdog instance;
    return instance;
}

EngineWatchdog::~EngineWatchdog() {
    Shutdown();
}

void EngineWatchdog::Initialize(uint32_t stallTimeoutMs) {
    if (m_Initialized.load()) {
        return;
    }

    if (const char* envTimeout = std::getenv("WE_WATCHDOG_TIMEOUT_MS")) {
        if (envTimeout[0] != '\0') {
            const int parsed = std::atoi(envTimeout);
            if (parsed > 100) {
                stallTimeoutMs = static_cast<uint32_t>(parsed);
            }
        }
    }

    m_StallTimeoutMs = stallTimeoutMs;
#if defined(_WIN32)
    m_MainThreadId = static_cast<uint32_t>(GetCurrentThreadId());
#endif
    m_LastHeartbeatTimeMs.store(GetCurrentTimeMs());

    {
        std::lock_guard<std::mutex> lock(m_StageMutex);
        m_CurrentStage = "Initialized";
    }

    m_Running.store(true);
    m_Initialized.store(true);
    m_WorkerThread = std::thread(&EngineWatchdog::WatchdogLoop, this);

    Logger::Log(Logger::Level::Info, LogCategory::General,
        "[EngineWatchdog] Initialized freeze detector (Stall threshold: " +
        std::to_string(m_StallTimeoutMs) + " ms, Main Thread ID: " +
        std::to_string(m_MainThreadId) + ")");
}

void EngineWatchdog::Shutdown() {
    if (!m_Initialized.load()) {
        return;
    }

    m_Running.store(false);
    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }
    m_Initialized.store(false);

    Logger::Log(Logger::Level::Info, LogCategory::General,
        "[EngineWatchdog] Watchdog thread shut down.");
}

void EngineWatchdog::Heartbeat(std::string_view stageName) {
    m_LastHeartbeatTimeMs.store(GetCurrentTimeMs());
    if (!stageName.empty()) {
        std::lock_guard<std::mutex> lock(m_StageMutex);
        m_CurrentStage = std::string(stageName);
    }
}

bool EngineWatchdog::IsStalled() const {
    if (!m_Initialized.load()) return false;
    const int64_t elapsed = GetCurrentTimeMs() - m_LastHeartbeatTimeMs.load();
    return elapsed >= static_cast<int64_t>(m_StallTimeoutMs);
}

uint32_t EngineWatchdog::GetStallDurationMs() const {
    if (!m_Initialized.load()) return 0;
    const int64_t elapsed = GetCurrentTimeMs() - m_LastHeartbeatTimeMs.load();
    return elapsed > 0 ? static_cast<uint32_t>(elapsed) : 0u;
}

std::string EngineWatchdog::GetCurrentStage() const {
    std::lock_guard<std::mutex> lock(m_StageMutex);
    return m_CurrentStage;
}

EngineWatchdog::Scoped::Scoped(std::string_view stageName) {
    auto& watchdog = EngineWatchdog::Get();
    m_PrevStage = watchdog.GetCurrentStage();
    watchdog.Heartbeat(stageName);
}

EngineWatchdog::Scoped::~Scoped() {
    EngineWatchdog::Get().Heartbeat(m_PrevStage);
}

void EngineWatchdog::WatchdogLoop() {
    while (m_Running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (!m_Running.load()) break;

        const int64_t nowMs = GetCurrentTimeMs();
        const int64_t lastMs = m_LastHeartbeatTimeMs.load();
        const int64_t elapsedMs = nowMs - lastMs;

        if (elapsedMs >= static_cast<int64_t>(m_StallTimeoutMs)) {
            // Rate-limit freeze incident reports to once every 3 seconds while continuously stalled
            if (nowMs - m_LastReportTimeMs >= 3000) {
                m_LastReportTimeMs = nowMs;
                std::string stage;
                {
                    std::lock_guard<std::mutex> lock(m_StageMutex);
                    stage = m_CurrentStage;
                }
                ReportFreezeIncident(static_cast<uint32_t>(elapsedMs), stage);
            }
        }
    }
}

void EngineWatchdog::CaptureMainThreadCallstack(std::vector<std::string>& outFrames) {
#if defined(_WIN32)
    if (m_MainThreadId == 0) return;

    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
        m_MainThreadId);
    if (!hThread || hThread == INVALID_HANDLE_VALUE) {
        outFrames.push_back("Failed to OpenThread on main thread ID " + std::to_string(m_MainThreadId));
        return;
    }

    DWORD suspendCount = SuspendThread(hThread);
    if (suspendCount == static_cast<DWORD>(-1)) {
        CloseHandle(hThread);
        outFrames.push_back("Failed to SuspendThread on main thread ID " + std::to_string(m_MainThreadId));
        return;
    }

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(hThread, &context)) {
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);

        STACKFRAME64 stackFrame{};
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_X64) || defined(__x86_64__)
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrFrame.Offset = context.Rbp;
        stackFrame.AddrStack.Offset = context.Rsp;
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrFrame.Offset = context.Ebp;
        stackFrame.AddrStack.Offset = context.Esp;
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        for (int i = 0; i < 32; ++i) {
            if (!StackWalk64(machineType, process, hThread, &stackFrame, &context, NULL,
                SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
            if (stackFrame.AddrPC.Offset == 0) break;

            DWORD64 displacement = 0;
            IMAGEHLP_LINE64 lineInfo{};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisplacement = 0;

            std::ostringstream ss;
            ss << "#" << std::setfill('0') << std::setw(2) << i << " ";
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, pSymbol)) {
                ss << pSymbol->Name;
                if (SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &lineDisplacement, &lineInfo)) {
                    ss << "  (" << lineInfo.FileName << ":" << lineInfo.LineNumber << ")";
                }
            } else {
                ss << "0x" << std::hex << stackFrame.AddrPC.Offset;
            }
            outFrames.push_back(ss.str());
        }
        SymCleanup(process);
    } else {
        outFrames.push_back("Failed to GetThreadContext for thread " + std::to_string(m_MainThreadId));
    }

    ResumeThread(hThread);
    CloseHandle(hThread);
#else
    outFrames.push_back("Callstack capture unsupported on non-Windows platforms.");
#endif
}

void EngineWatchdog::ReportFreezeIncident(uint32_t stallMs, const std::string& stage) {
    if (m_InStallReport.exchange(true)) return;

    ++m_FreezeIncidentCount;
    std::vector<std::string> frames;
    CaptureMainThreadCallstack(frames);

    std::ostringstream report;
    report << "\n======================================================================\n";
    report << "!!! [ENGINE WATCHDOG FREEZE DETECTED] Incident #" << m_FreezeIncidentCount << " !!!\n";
    report << "Main Thread (ID: " << m_MainThreadId << ") has STALLED / FROZEN!\n";
    report << "Stall Duration: " << stallMs << " ms (Threshold: " << m_StallTimeoutMs << " ms)\n";
    report << "Last Recorded Stage: '" << stage << "'\n";
    report << "----------------------------------------------------------------------\n";
    report << "Main Thread Live Callstack:\n";
    if (frames.empty()) {
        report << "  <No stack frames captured>\n";
    } else {
        for (const auto& frame : frames) {
            report << "  " << frame << "\n";
        }
    }
    report << "======================================================================\n\n";

    const std::string text = report.str();

    // 1. Output directly to stderr & stdout so it shows immediately in terminal / CDB
    std::cerr << text << std::flush;

    // 2. Log via Engine Logger
    Logger::Log(Logger::Level::Critical, LogCategory::General,
        "[EngineWatchdog] FREEZE DETECTED! Duration: " + std::to_string(stallMs) +
        " ms | Stage: '" + stage + "' | Frames: " + std::to_string(frames.size()));

    // 3. Write dedicated Freeze Report artifact file
    try {
        const auto& logsRoot = Logger::GetLogsRoot();
        if (!logsRoot.empty()) {
            const auto reportPath = std::filesystem::path(logsRoot) /
                ("FreezeReport_Incident_" + std::to_string(m_FreezeIncidentCount) + ".txt");
            std::ofstream f(reportPath, std::ios::out | std::ios::trunc);
            if (f.is_open()) {
                f << text;
                f.close();
                Logger::Log(Logger::Level::Info, LogCategory::General,
                    "[EngineWatchdog] Wrote freeze diagnosis report to: " + reportPath.string());
            }
        }
    } catch (...) {
        // Suppress any file system exceptions in watchdog
    }

    Logger::Flush();
    m_InStallReport.store(false);
}

void EngineWatchdog::DumpMainThreadCallstack(const std::string& reason) {
    std::string stage;
    {
        std::lock_guard<std::mutex> lock(m_StageMutex);
        stage = m_CurrentStage;
    }
    ReportFreezeIncident(GetStallDurationMs(), stage + " [Manual Dump: " + reason + "]");
}

}
