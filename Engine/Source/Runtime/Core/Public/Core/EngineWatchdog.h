// ==============================================================================
// WindEffects — Core — EngineWatchdog
// Automated heartbeat monitoring and main-thread stall/freeze detection.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::core {

/// Background watchdog system that tracks main-thread execution heartbeats.
/// If the main thread stalls for longer than the stall threshold (default: 2000 ms),
/// the watchdog triggers a freeze alert and captures a live callstack of the main thread.
class CORE_API EngineWatchdog {
public:
    static EngineWatchdog& Get();

    /// Initializes the watchdog background thread and sets stall timeout in milliseconds.
    void Initialize(uint32_t stallTimeoutMs = 2000);

    /// Shuts down the watchdog monitoring thread safely.
    void Shutdown();

    /// Signals a heartbeat from the main thread at a named pipeline stage.
    void Heartbeat(std::string_view stageName = {});

    /// Returns true if the main thread is currently considered stalled (heartbeat overdue).
    [[nodiscard]] bool IsStalled() const;

    /// Returns elapsed time in ms since the last main-thread heartbeat.
    [[nodiscard]] uint32_t GetStallDurationMs() const;

    /// Returns the name of the last recorded pipeline stage.
    [[nodiscard]] std::string GetCurrentStage() const;

    /// Manually triggers a diagnostic callstack capture of the main thread.
    void DumpMainThreadCallstack(const std::string& reason);

    /// Scoped heartbeat helper for tracking nested block execution durations.
    struct CORE_API Scoped {
        explicit Scoped(std::string_view stageName);
        ~Scoped();
        Scoped(const Scoped&) = delete;
        Scoped& operator=(const Scoped&) = delete;
    private:
        std::string m_PrevStage;
    };

private:
    EngineWatchdog() = default;
    ~EngineWatchdog();

    void WatchdogLoop();
    void CaptureMainThreadCallstack(std::vector<std::string>& outFrames);
    void ReportFreezeIncident(uint32_t stallMs, const std::string& stage);

    std::thread m_WorkerThread;
    std::atomic<bool> m_Running{ false };
    std::atomic<bool> m_Initialized{ false };

    uint32_t m_MainThreadId{ 0 };
    uint32_t m_StallTimeoutMs{ 2000 };

    std::atomic<int64_t> m_LastHeartbeatTimeMs{ 0 };
    mutable std::mutex m_StageMutex;
    std::string m_CurrentStage{ "Uninitialized" };

    std::atomic<bool> m_InStallReport{ false };
    int64_t m_LastReportTimeMs{ 0 };
    int32_t m_FreezeIncidentCount{ 0 };
};

} // namespace we::runtime::core

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
