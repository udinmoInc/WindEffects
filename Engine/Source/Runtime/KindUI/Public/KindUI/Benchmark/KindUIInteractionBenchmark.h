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

struct KINDUI_API UiLatencyScenarioResult {
    std::string name;
    double inputToHandlerMs = 0.0;
    double handlerToPaintMs = 0.0;
    double paintToPresentMs = 0.0;
    double totalVisibleMs = 0.0;
    std::string rootCause;
};

struct KINDUI_API UiLatencyBenchmarkReport {
    std::vector<UiLatencyScenarioResult> scenarios;
    std::string summary;
};

/// Headless interaction spike profiler for docking, splitters, popups, resize.
[[nodiscard]] KINDUI_API KindUIInteractionReport RunKindUIInteractionBenchmark(uint32_t dragSteps = 32);

/// Simulates editor input→layout→paint→present path with UiInputLatencyAudit.
[[nodiscard]] KINDUI_API UiLatencyBenchmarkReport RunUiInputLatencyBenchmark(uint32_t steps = 32);

struct KINDUI_API HitTestAuditResult {
    std::string name;
    bool passed = false;
    std::string detail;
};

struct KINDUI_API HitTestAuditReport {
    std::vector<HitTestAuditResult> cases;
    std::string summary;
};

/// Validates framework hit-testing: clipping, z-order, scroll offset, splitters.
[[nodiscard]] KINDUI_API HitTestAuditReport RunHitTestAudit();

} // namespace we::runtime::kindui
