#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::kindui {

struct KINDUI_API InteractionScenarioResult {
    std::string name;
    double peakMs = 0.0;
    double avgMs = 0.0;
    uint32_t layoutPasses = 0;
    uint32_t paintPasses = 0;
    uint32_t invalidateCount = 0;
    uint64_t layoutInvalidations = 0;
    uint64_t paintInvalidations = 0;
    uint32_t widgetsVisited = 0;
    uint32_t paintCommands = 0;
    uint32_t geometryVertices = 0;
    std::string rootCause;
};

struct KINDUI_API KindUIInteractionReport {
    std::vector<InteractionScenarioResult> scenarios;
    InteractionScenarioResult idleBaseline{};
    std::string summary;
};

/// Headless interaction spike profiler for docking, splitters, popups, resize.
[[nodiscard]] KINDUI_API KindUIInteractionReport RunKindUIInteractionBenchmark(uint32_t dragSteps = 32);

} // namespace we::runtime::kindui
