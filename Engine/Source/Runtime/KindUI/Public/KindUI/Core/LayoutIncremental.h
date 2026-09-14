// ==============================================================================
// WindEffects — KindUI — LayoutIncremental
// Shared incremental Measure/Arrange helpers. Containers call MeasureChild /
// ArrangeChild so clean subtrees are skipped automatically — no per-panel hacks.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Per-layout-pass counters (reset at the start of each host/adapter layout).
struct KINDUI_API LayoutIncrementalStats {
    uint32_t measureAttempts = 0;
    uint32_t measureSkipped = 0;
    uint32_t measureRan = 0;
    uint32_t arrangeAttempts = 0;
    uint32_t arrangeSkipped = 0;
    uint32_t arrangeRan = 0;
    uint32_t fullLayoutPasses = 0;

    void Reset() { *this = {}; }

    static LayoutIncrementalStats& Current();
    static void ResetCurrent();
};

[[nodiscard]] inline bool SizeApproxEqual(float a, float b, float eps = 0.5f) noexcept {
    const float d = a - b;
    return d <= eps && d >= -eps;
}

} // namespace we::runtime::kindui
