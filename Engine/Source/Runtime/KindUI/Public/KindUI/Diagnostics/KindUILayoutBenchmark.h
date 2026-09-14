// ==============================================================================
// WindEffects — KindUI — KindUILayoutBenchmark
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

struct KINDUI_API LayoutScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    double avgMeasureArrangeMicros = 0.0;
    double avgFrameMicros = 0.0;
    uint32_t avgMeasureRan = 0;
    uint32_t avgMeasureSkipped = 0;
    uint32_t avgArrangeRan = 0;
    uint32_t avgArrangeSkipped = 0;
    uint32_t fullLayoutPasses = 0;
    uint32_t affectedWidgetsApprox = 0;
};

struct KINDUI_API KindUILayoutReport {
    std::vector<LayoutScenarioResult> scenarios;
    std::string summary;
};

/// Headless incremental layout bench (property / selection / expand / scroll / resize / large tree).
[[nodiscard]] KINDUI_API KindUILayoutReport RunKindUILayoutBenchmark(uint32_t iterations = 64);

} // namespace we::runtime::kindui
