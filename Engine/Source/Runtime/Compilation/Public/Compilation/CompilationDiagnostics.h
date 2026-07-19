#pragma once

#include "Compilation/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::runtime::compilation {

struct COMPILATION_API CompilationDiagnosticsSnapshot {
    std::uint64_t requestsSubmitted = 0;
    std::uint64_t jobsCompleted = 0;
    std::uint64_t jobsFailed = 0;
    std::uint64_t cacheHits = 0;
    std::uint64_t cacheMisses = 0;
    std::uint64_t invalidations = 0;
    std::uint64_t compileMicros = 0;
    std::uint64_t queueDepthPeak = 0;
};

class COMPILATION_API CompilationDiagnostics {
public:
    static CompilationDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] CompilationDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnSubmit() noexcept;
    void OnComplete(std::uint64_t micros, bool cacheHit, bool failed) noexcept;
    void OnInvalidate() noexcept;
    void OnQueueDepth(std::uint64_t depth) noexcept;

private:
    std::atomic<std::uint64_t> m_Submitted{0};
    std::atomic<std::uint64_t> m_Completed{0};
    std::atomic<std::uint64_t> m_Failed{0};
    std::atomic<std::uint64_t> m_CacheHits{0};
    std::atomic<std::uint64_t> m_CacheMisses{0};
    std::atomic<std::uint64_t> m_Invalidations{0};
    std::atomic<std::uint64_t> m_CompileMicros{0};
    std::atomic<std::uint64_t> m_QueuePeak{0};
};

} // namespace we::runtime::compilation
