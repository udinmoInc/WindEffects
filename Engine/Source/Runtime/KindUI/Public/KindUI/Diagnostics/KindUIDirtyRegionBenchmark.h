// ==============================================================================
// WindEffects — KindUI — KindUIDirtyRegionBenchmark
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

struct KINDUI_API DirtyRegionScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    float avgDirtyCoverage = 0.0f;
    float avgDirtyAreaPx = 0.0f;
    uint32_t avgDirtyRegions = 0;
    double avgPaintMicros = 0.0;
    double avgDrawgenMicros = 0.0;
    double avgFrameMicros = 0.0;
    uint32_t geometryReuseHits = 0;
    uint32_t paintCommands = 0;
    uint32_t drawCommands = 0;
};

struct KINDUI_API KindUIDirtyRegionReport {
    std::vector<DirtyRegionScenarioResult> scenarios;
    std::string summary;
};

/// Headless dirty-region + retention/drawgen reuse bench (selection/hover/scroll/expand).
[[nodiscard]] KINDUI_API KindUIDirtyRegionReport RunKindUIDirtyRegionBenchmark(
    uint32_t iterations = 64);

} // namespace we::runtime::kindui
