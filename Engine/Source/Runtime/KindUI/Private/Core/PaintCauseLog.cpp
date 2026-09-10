// ==============================================================================
// WindEffects — KindUI — PaintCauseLog
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Profiling/PaintCauseLog.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#endif

namespace we::runtime::kindui {

namespace {

constexpr size_t kRingSize = 512;

struct RawCause {
    uint64_t seq = 0;
    double ms = 0.0;
    void* caller = nullptr;
    char kind[16]{};
};

double NowMs() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::milli>(clock::now().time_since_epoch()).count();
}

bool EnvOff(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] == '0';
}

} // namespace

struct PaintCauseLogState {
    std::mutex mutex;
    RawCause ring[kRingSize]{};
    size_t head = 0;
    uint64_t seq = 0;
    uint64_t dropped = 0;
    size_t drainCursor = 0; // ring position already drained
    uint64_t drainSeq = 0;
    double startMs = 0.0;
    bool enabled = false;
    bool enabledChecked = false;
    std::unordered_map<void*, std::string> resolveCache;
#if defined(_WIN32)
    bool symReady = false;
#endif
};

static PaintCauseLogState& State() {
    static PaintCauseLogState state;
    return state;
}

PaintCauseLog& PaintCauseLog::Get() {
    static PaintCauseLog instance;
    return instance;
}

bool PaintCauseLog::IsEnabled() {
    PaintCauseLogState& s = State();
    if (!s.enabledChecked) {
        // Default OFF — symbol resolve + ring drain under mutex can stall the UI
        // when every hover/anim invalidation is recorded. Opt in with WE_PAINT_CAUSE=1,
        // or automatically when WE_SCREEN_DEBUG is on for the live debugger.
        const char* paint = std::getenv("WE_PAINT_CAUSE");
        const char* screen = std::getenv("WE_SCREEN_DEBUG");
        const bool paintOn = paint != nullptr && paint[0] == '1';
        const bool paintExplicitOff = EnvOff("WE_PAINT_CAUSE");
        const bool screenOn = screen != nullptr && screen[0] != '\0' && screen[0] != '0';
        s.enabled = paintOn || (screenOn && !paintExplicitOff);
        s.enabledChecked = true;
    }
    return s.enabled;
}

void PaintCauseLog::Push(const char* kind, void* caller) {
    PaintCauseLogState& s = State();
    if (!IsEnabled()) {
        return;
    }
    const double now = NowMs();
    std::lock_guard<std::mutex> lock(s.mutex);
    if (s.startMs == 0.0) {
        s.startMs = now;
    }
    RawCause& slot = s.ring[s.head];
    if (slot.seq > s.drainSeq) {
        ++s.dropped; // overwritten before it was ever drained
    }
    slot.seq = ++s.seq;
    slot.ms = now - s.startMs;
    slot.caller = caller;
    if (kind != nullptr) {
        std::snprintf(slot.kind, sizeof(slot.kind), "%s", kind);
    } else {
        slot.kind[0] = '\0';
    }
    s.head = (s.head + 1) % kRingSize;
}

uint64_t PaintCauseLog::Dropped() const {
    PaintCauseLogState& s = State();
    std::lock_guard<std::mutex> lock(s.mutex);
    return s.dropped;
}

namespace {

std::string ResolveCaller(PaintCauseLogState& s, void* caller) {
    if (caller == nullptr) {
        return "unknown";
    }
    auto cached = s.resolveCache.find(caller);
    if (cached != s.resolveCache.end()) {
        return cached->second;
    }
    std::string resolved;
#if defined(_WIN32)
    if (!s.symReady) {
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        s.symReady = true;
    }
    char buffer[sizeof(SYMBOL_INFO) + 512 * sizeof(char)];
    PSYMBOL_INFO sym = reinterpret_cast<PSYMBOL_INFO>(buffer);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 511;
    DWORD64 displacement = 0;
    if (SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(caller), &displacement, sym)) {
        char offset[32];
        std::snprintf(offset, sizeof(offset), "+0x%llx", static_cast<unsigned long long>(displacement));
        resolved = std::string(sym->Name) + offset;
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(GetCurrentProcess(), reinterpret_cast<DWORD64>(caller), &lineDisplacement, &line)) {
            const char* file = std::strrchr(line.FileName, '\\');
            file = (file != nullptr) ? file + 1 : line.FileName;
            const char* slash = std::strrchr(file, '/');
            file = (slash != nullptr) ? slash + 1 : file;
            char loc[64];
            std::snprintf(loc, sizeof(loc), " %s:%lu", file, static_cast<unsigned long>(line.LineNumber));
            resolved += loc;
        }
    } else {
        char raw[32];
        std::snprintf(raw, sizeof(raw), "addr:%p", caller);
        resolved = raw;
    }
#else
    char raw[32];
    std::snprintf(raw, sizeof(raw), "addr:%p", caller);
    resolved = raw;
#endif
    s.resolveCache[caller] = resolved;
    return resolved;
}

} // namespace

std::vector<PaintCauseLog::ResolvedCause> PaintCauseLog::Drain() {
    PaintCauseLogState& s = State();
    std::lock_guard<std::mutex> lock(s.mutex);
    std::vector<ResolvedCause> out;
    if (s.seq == s.drainSeq) {
        return out;
    }
    // Walk the ring in seq order, emitting only entries newer than drainSeq.
    // Collect matching slots first (ring order != seq order after wrap).
    for (size_t i = 0; i < kRingSize; ++i) {
        const RawCause& slot = s.ring[i];
        if (slot.seq > s.drainSeq) {
            ResolvedCause r;
            r.seq = slot.seq;
            r.ms = slot.ms;
            r.kind = slot.kind;
            r.caller = ResolveCaller(s, slot.caller);
            out.push_back(std::move(r));
        }
    }
    // Slots never written have seq 0 and are skipped by the drainSeq check
    // once any entry exists (drainSeq starts 0, first real seq is 1).
    std::sort(out.begin(), out.end(),
        [](const ResolvedCause& a, const ResolvedCause& b) { return a.seq < b.seq; });
    if (!out.empty()) {
        s.drainSeq = out.back().seq;
    }
    return out;
}

} // namespace we::runtime::kindui
 
