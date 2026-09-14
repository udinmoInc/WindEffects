// ==============================================================================
// WindEffects — KindUI — KindUIStateChangeBenchmark
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

struct KINDUI_API StateChangeScenarioResult {
    std::string name;
    uint32_t iterations = 0;
    double avgUpdateMicros = 0.0;
    double avgFrameMicros = 0.0;
    uint32_t avgNotifications = 0;
    uint32_t avgDeduped = 0;
    uint32_t avgWidgetsPaint = 0;
    uint32_t avgWidgetsLayout = 0;
    uint32_t avgGatePaintArms = 0;
    uint32_t avgGateLayoutArms = 0;
    uint32_t avgTransactions = 0;
    uint32_t avgMeasureRan = 0;
    uint32_t avgArrangeRan = 0;
    uint32_t avgLayoutPasses = 0;
};

struct KINDUI_API KindUIStateChangeReport {
    std::vector<StateChangeScenarioResult> scenarios;
    std::string summary;
};

/// Headless state-change bench (hover / focus / selection / property / expansion / multi-select).
[[nodiscard]] KINDUI_API KindUIStateChangeReport RunKindUIStateChangeBenchmark(uint32_t iterations = 64);

} // namespace we::runtime::kindui
