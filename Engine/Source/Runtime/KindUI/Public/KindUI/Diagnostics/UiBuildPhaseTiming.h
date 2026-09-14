#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Per-rebuild CPU phase timings for ProcessWidget (milliseconds).
/// Populated when WE_UI_BUILD_PROFILE=1 or always (cheap; logged only when enabled).
struct KINDUI_API UiBuildPhaseTiming {
    float clearMs = 0.0f;
    float layoutMs = 0.0f;   // Measure+Arrange inside adapter (usually 0; host owns layout)
    float paintMs = 0.0f;    // Widget::Paint + DrawCommand recording
    float coalesceMs = 0.0f; // DrawCommandBatcher::CoalesceCommands
    float drawgenMs = 0.0f;  // ConvertDrawCommand (rects/icons/…)
    float batchCoalesceMs = 0.0f; // DrawCommandBatcher::CoalesceBatches
    float textMs = 0.0f;     // Text path inside drawgen
    float clearDirtyMs = 0.0f;
    float totalMs = 0.0f;
    uint32_t paintCommands = 0;          // raw commands after paint (before coalesce)
    uint32_t commandsAfterCoalesce = 0;  // after command coalesce
    uint32_t commandsDropped = 0;
    uint32_t rectsMerged = 0;
    uint32_t clipsNormalized = 0;
    uint32_t iconsClustered = 0;
    uint32_t textsClustered = 0;
    uint32_t textCommands = 0;
    uint32_t rectCommands = 0;
    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t batchesAfterDrawgen = 0; // adjacent merge only (legacy baseline)
    uint32_t batches = 0;             // after global batch coalesce
    uint32_t batchesMerged = 0;
    uint32_t textureSwitches = 0;
    uint32_t clipRectCount = 0;
    uint32_t subtreesPainted = 0;
    uint32_t subtreesReplayed = 0;
    uint32_t commandsReplayed = 0;
    uint32_t dirtyRegionCount = 0;
    uint32_t dirtyMergedCount = 0;
    float dirtyCoverage = 0.0f;
    float dirtyAreaPx = 0.0f;
    bool dirtyFull = false;
    bool geometryReused = false;
    uint32_t layoutMeasureRan = 0;
    uint32_t layoutMeasureSkipped = 0;
    uint32_t layoutArrangeRan = 0;
    uint32_t layoutArrangeSkipped = 0;
    uint32_t layoutFullPasses = 0;
    bool ranLayout = false;
    bool paintRetention = false;
    bool globalBatchEnabled = true;
};

} // namespace we::runtime::kindui
