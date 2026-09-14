// ==============================================================================
// WindEffects — KindUI — KindUIResidencyBenchmark
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

class OverlayRenderer;

struct KINDUI_API ResidencyScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    double avgFrameMicros = 0.0;
    uint32_t residentIcons = 0;
    uint64_t residentIconGpuBytes = 0;
    uint32_t residentTextGeom = 0;
    uint64_t residentTextGeomBytes = 0;
    uint64_t loads = 0;
    uint64_t uploads = 0;
    uint64_t evictions = 0;
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint64_t deferredReleases = 0;
};

struct KINDUI_API KindUIResidencyReport {
    std::vector<ResidencyScenarioResult> scenarios;
    std::string summary;
};

/// Headless residency bench (idle / icon / glyph / open-close / scroll / overlay /
/// pressure eviction / re-request after eviction).
[[nodiscard]] KINDUI_API KindUIResidencyReport RunKindUIResidencyBenchmark(
    OverlayRenderer* overlay,
    uint32_t iterations = 48);

} // namespace we::runtime::kindui
