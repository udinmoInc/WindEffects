#pragma once

#include "Platform/Export.h"

#include <cstdint>

namespace we::platform {

// Cumulative and last-frame platform diagnostics for profiling / production telemetry.
struct PLATFORM_API PlatformDiagnostics {
    // Lifecycle
    double initializeTimeMs = 0.0;
    double shutdownTimeMs = 0.0;

    // Windowing
    uint64_t windowsCreated = 0;
    uint64_t windowsDestroyed = 0;
    double lastWindowCreateMs = 0.0;
    double totalWindowCreateMs = 0.0;

    // Events / input
    uint64_t eventsDispatched = 0;
    uint64_t eventsCoalesced = 0;
    double lastPollEventsMs = 0.0;
    double totalPollEventsMs = 0.0;
    double estimatedInputLatencyMs = 0.0; // best-effort; may be zero if unsupported

    // Timers
    double timerPrecisionNs = 0.0;
    uint64_t timerFrequencyHz = 0;

    // Threading
    uint64_t threadsCreated = 0;
    double lastThreadCreateMs = 0.0;

    // Directory watching
    uint64_t directoryWatchersCreated = 0;
    uint64_t directoryNotifications = 0;
    double lastDirectoryPollMs = 0.0;

    void Reset() { *this = PlatformDiagnostics{}; }
};

} // namespace we::platform
