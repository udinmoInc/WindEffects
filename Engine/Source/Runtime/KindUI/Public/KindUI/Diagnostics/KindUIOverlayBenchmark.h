// ==============================================================================
// WindEffects — KindUI — KindUIOverlayBenchmark
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

namespace we::runtime::kindui {

struct KINDUI_API OverlayScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    double avgUpdateMicros = 0.0;
    double avgFrameMicros = 0.0;
    uint32_t avgOverlayOnlyLayouts = 0;
    uint32_t avgFullHostArranges = 0;
    uint32_t avgSlotsMeasured = 0;
    uint32_t avgSlotsArranged = 0;
    uint32_t avgBaseArrangesSkipped = 0;
    uint32_t avgRepositionSkipped = 0;
    uint32_t avgMeasureRan = 0;
    uint32_t avgArrangeRan = 0;
    float avgDirtyCoverage = 0.0f;
    uint32_t avgDirtyRegions = 0;
};

struct KINDUI_API KindUIOverlayReport {
    std::vector<OverlayScenarioResult> scenarios;
    std::string summary;
};

/// Headless overlay compositor bench (idle / dropdown / context / tooltip /
/// scroll reposition / stacked / open-close / large base isolation).
[[nodiscard]] KINDUI_API KindUIOverlayReport RunKindUIOverlayBenchmark(uint32_t iterations = 64);

} // namespace we::runtime::kindui
