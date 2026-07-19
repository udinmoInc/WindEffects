#include "Compilation/CompilationDiagnostics.h"

#include <sstream>

namespace we::runtime::compilation {

CompilationDiagnostics& CompilationDiagnostics::Get() noexcept {
    static CompilationDiagnostics instance;
    return instance;
}

void CompilationDiagnostics::Reset() noexcept {
    m_Submitted.store(0);
    m_Completed.store(0);
    m_Failed.store(0);
    m_CacheHits.store(0);
    m_CacheMisses.store(0);
    m_Invalidations.store(0);
    m_CompileMicros.store(0);
    m_QueuePeak.store(0);
}

CompilationDiagnosticsSnapshot CompilationDiagnostics::Snapshot() const noexcept {
    CompilationDiagnosticsSnapshot s;
    s.requestsSubmitted = m_Submitted.load();
    s.jobsCompleted = m_Completed.load();
    s.jobsFailed = m_Failed.load();
    s.cacheHits = m_CacheHits.load();
    s.cacheMisses = m_CacheMisses.load();
    s.invalidations = m_Invalidations.load();
    s.compileMicros = m_CompileMicros.load();
    s.queueDepthPeak = m_QueuePeak.load();
    return s;
}

std::string CompilationDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "Compilation submitted=" << s.requestsSubmitted
        << " completed=" << s.jobsCompleted
        << " failed=" << s.jobsFailed
        << " cacheHits=" << s.cacheHits
        << " cacheMisses=" << s.cacheMisses
        << " invalidations=" << s.invalidations
        << " compileUs=" << s.compileMicros
        << " queuePeak=" << s.queueDepthPeak;
    return oss.str();
}

void CompilationDiagnostics::OnSubmit() noexcept { m_Submitted.fetch_add(1); }

void CompilationDiagnostics::OnComplete(std::uint64_t micros, bool cacheHit, bool failed) noexcept {
    m_CompileMicros.fetch_add(micros);
    if (failed) {
        m_Failed.fetch_add(1);
        return;
    }
    m_Completed.fetch_add(1);
    if (cacheHit) {
        m_CacheHits.fetch_add(1);
    } else {
        m_CacheMisses.fetch_add(1);
    }
}

void CompilationDiagnostics::OnInvalidate() noexcept { m_Invalidations.fetch_add(1); }

void CompilationDiagnostics::OnQueueDepth(std::uint64_t depth) noexcept {
    auto cur = m_QueuePeak.load();
    while (depth > cur && !m_QueuePeak.compare_exchange_weak(cur, depth)) {
    }
}

} // namespace we::runtime::compilation
