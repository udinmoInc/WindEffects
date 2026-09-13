// ==============================================================================
// WindEffects — KindUI — PaintCauseLog
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>
#include <vector>

// Exact-caller capture for every paint/layout invalidation path.
// Hooked into Widget::InvalidatePaint and UIRepaintGate::Request* — no call
// sites needed changes. No RTTI dependency: callers are captured as return
// addresses and resolved to function+line via DbgHelp at drain time
// (Windows dev builds; other platforms record raw addresses).
// Default OFF. Enable with WE_PAINT_CAUSE=1 (also auto-on when WE_SCREEN_DEBUG
// is set, unless WE_PAINT_CAUSE=0). Pushes are bounded: widget invalidations
// early-out when already dirty, and the ring overwrites.
#if defined(_MSC_VER)
#include <intrin.h>
#define WE_PAINT_CALLER _ReturnAddress()
#else
#define WE_PAINT_CALLER __builtin_return_address(0)
#endif

namespace we::runtime::kindui {

class KINDUI_API PaintCauseLog {
public:
    static PaintCauseLog& Get();

    static bool IsEnabled();

    // kind: "invalidate" (widget funnel) | "gate-paint" | "gate-layout"
    //       | "gate-all" | "gate-anim". caller: WE_PAINT_CALLER address.
    void Push(const char* kind, void* caller);

    struct ResolvedCause {
        uint64_t seq = 0;
        double ms = 0.0; // ms since first recorded cause
        std::string kind;
        std::string caller; // "Module!Function+offset File:line" (Windows)
    };

    // Entries recorded since the previous Drain (symbol-resolved, cached).
    [[nodiscard]] std::vector<ResolvedCause> Drain();

    [[nodiscard]] uint64_t Dropped() const;

private:
    PaintCauseLog() = default;
};

} // namespace we::runtime::kindui
