// ==============================================================================
// WindEffects — KindUI — KindUITextBenchmark
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

class TextUIService;

struct KINDUI_API TextScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    double avgMeasureUs = 0.0;
    double avgLayoutUs = 0.0;
    double avgDrawgenUs = 0.0;
    double avgFrameUs = 0.0;
    uint32_t avgLayoutHits = 0;
    uint32_t avgLayoutMisses = 0;
    uint32_t avgGeomHits = 0;
    uint32_t avgGeomMisses = 0;
    uint32_t avgGlyphs = 0;
    uint32_t avgTextDraws = 0;
    uint32_t avgAtlasUploads = 0;
    uint32_t avgBatches = 0;
    uint32_t avgTextureSwitches = 0;
    uint64_t avgLayoutCacheBytes = 0;
};

struct KINDUI_API KindUITextReport {
    std::vector<TextScenarioResult> scenarios;
    std::string summary;
};

/// Headless text path bench: cold/warm/repeated/many labels/scroll/selection/icons+text.
/// Pass the live editor TextUIService for layout/geometry/atlas stats (required for drawgen scenarios).
[[nodiscard]] KINDUI_API KindUITextReport RunKindUITextBenchmark(
    TextUIService* textService,
    uint32_t iterations = 64);

} // namespace we::runtime::kindui
