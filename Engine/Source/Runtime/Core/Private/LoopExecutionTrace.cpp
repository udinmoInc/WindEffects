// ==============================================================================
// WindEffects — Core — LoopExecutionTrace
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/LoopExecutionTrace.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>

namespace we::runtime::core {
namespace {

using Clock = std::chrono::steady_clock;

struct TraceTls {
    std::unordered_map<std::string, Clock::time_point> open;
};

TraceTls& Tls() {
    thread_local TraceTls tls;
    return tls;
}

bool EnvEnabled() {
    static const bool enabled = []() {
        const char* v = std::getenv("WE_LOOP_TRACE");
        if (!v || !*v) {
            return false;
        }
        return std::strcmp(v, "0") != 0 && std::strcmp(v, "false") != 0;
    }();
    return enabled;
}

double ElapsedMs(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void Emit(const char* phase, std::string_view name, std::string_view detail, double elapsedMs = -1.0) {
    std::ostringstream line;
    line << "[LoopTrace] " << phase << ' ' << name;
    if (elapsedMs >= 0.0) {
        line << " | elapsedMs=" << elapsedMs;
    }
    if (!detail.empty()) {
        line << " | " << detail;
    }
    WE_LOG_INFO(we::LogCategory::Diagnostics.data(), line.str());
}

} // namespace

bool LoopExecutionTrace::IsEnabled() {
    return EnvEnabled();
}

void LoopExecutionTrace::Enter(std::string_view name, std::string_view detail) {
    if (!EnvEnabled()) {
        return;
    }
    Tls().open[std::string(name)] = Clock::now();
    Emit("ENTER", name, detail);
}

void LoopExecutionTrace::Exit(std::string_view name, std::string_view detail) {
    if (!EnvEnabled()) {
        return;
    }
    double ms = -1.0;
    auto& open = Tls().open;
    const auto it = open.find(std::string(name));
    if (it != open.end()) {
        ms = ElapsedMs(it->second);
        open.erase(it);
    }
    Emit("EXIT ", name, detail, ms);
}

void LoopExecutionTrace::Event(std::string_view name, std::string_view detail) {
    if (!EnvEnabled()) {
        return;
    }
    Emit("EVENT", name, detail);
}

void LoopExecutionTrace::MutexWait(std::string_view name, double waitMs, double warnMs) {
    if (!EnvEnabled() || waitMs < warnMs) {
        return;
    }
    std::ostringstream detail;
    detail << "waitMs=" << waitMs << " (threshold=" << warnMs << ")";
    Emit("MUTEX", name, detail.str(), waitMs);
}

void LoopExecutionTrace::GateState(
    std::string_view where,
    bool needsLayout,
    bool needsPaint,
    size_t frameEvents,
    size_t pendingHint) {
    if (!EnvEnabled()) {
        return;
    }
    std::ostringstream detail;
    detail << "layout=" << (needsLayout ? 1 : 0)
           << " paint=" << (needsPaint ? 1 : 0)
           << " frameEvents=" << frameEvents
           << " pendingHint=" << pendingHint;
    Emit("GATE ", where, detail.str());
}

LoopExecutionTrace::Scoped::Scoped(std::string_view name, std::string_view detail)
    : m_Name(name)
    , m_Active(EnvEnabled()) {
    if (m_Active) {
        Enter(m_Name, detail);
    }
}

LoopExecutionTrace::Scoped::~Scoped() {
    if (m_Active && !m_Name.empty()) {
        Exit(m_Name);
    }
}

} // namespace we::runtime::core
