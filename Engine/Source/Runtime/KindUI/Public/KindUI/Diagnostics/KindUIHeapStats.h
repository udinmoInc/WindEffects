// ==============================================================================
// WindEffects — KindUI — KindUIHeapStats
// Optional KindUI-module heap attribution (Development / WE_KINDUI_HEAP=1).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Process + KindUI structural snapshot.
/// Process fields always come from the OS.
/// KindUI module heap attribution via global operator new is not measured
/// (unsafe across DLL boundaries); use process deltas + structural fields.
struct KINDUI_API KindUIMemorySnapshot {
    uint64_t processWorkingSetBytes = 0;
    uint64_t processPrivateBytes = 0;
    uint64_t processPeakWorkingSetBytes = 0;

    bool kinduiHeapTracking = false; // always false; reserved for future safe tracking
    uint64_t kinduiAllocCount = 0;
    uint64_t kinduiFreeCount = 0;
    uint64_t kinduiOutstandingBytes = 0;
    uint64_t kinduiPeakOutstandingBytes = 0;

    uint32_t widgetCount = 0;
    uint32_t visibleWidgetCount = 0;
    uint64_t widgetObjectBytesEstimate = 0; // widgetCount * sizeof(Widget) (base only)
    uint64_t retainedPaintBytesEstimate = 0;
    uint64_t geometryVertexCapacityBytes = 0;
    uint64_t geometryIndexCapacityBytes = 0;
    uint64_t geometryBatchCapacityBytes = 0;
    uint64_t drawCommandCapacityBytes = 0;
    uint64_t gpuVertexCapacityBytes = 0;
    uint64_t gpuIndexCapacityBytes = 0;
    uint32_t textMeasureCacheEntries = 0;
    uint64_t textMeasureCacheBytes = 0;
    uint32_t textMetricsCacheEntries = 0;
    uint64_t textMetricsCacheBytes = 0;
    uint32_t fontAtlasPageCount = 0;
    uint64_t fontAtlasCpuBytes = 0;
    uint32_t iconTextureCacheEntries = 0;
    uint64_t iconTextureCacheBytes = 0;
    uint32_t submissionCacheSlots = 0;
    uint32_t residencyIconCount = 0;
    uint64_t residencyIconGpuBytes = 0;
    uint32_t residencyTextGeomCount = 0;
    uint64_t residencyTextGeomBytes = 0;
    uint32_t residencyGlyphCount = 0;
    uint64_t residencyLoads = 0;
    uint64_t residencyUploads = 0;
    uint64_t residencyEvictions = 0;
    uint64_t residencyCacheHits = 0;
    uint64_t residencyCacheMisses = 0;
    uint64_t residencyDeferredReleases = 0;
    /// Sum of measured KindUI CPU structural fields (not process RSS, not GPU).
    uint64_t kinduiCpuStructuralBytes = 0;
};

class OverlayRenderer;
class Widget;

class KINDUI_API KindUIHeapStats {
public:
    /// Runtime gate (cached). Requires WE_KINDUI_HEAP_TRACK at compile time.
    [[nodiscard]] static bool IsTrackingEnabled();
    static void RefreshEnabledFromEnv();

    [[nodiscard]] static uint64_t AllocCount();
    [[nodiscard]] static uint64_t FreeCount();
    [[nodiscard]] static uint64_t OutstandingBytes();
    [[nodiscard]] static uint64_t PeakOutstandingBytes();
    static void ResetPeaks();

    /// Called from KindUI operator new/delete when tracking is compiled in.
    static void OnAlloc(size_t bytes);
    static void OnFree(size_t bytes);

    /// Capture process memory + optional KindUI heap + optional tree/geometry stats.
    [[nodiscard]] static KindUIMemorySnapshot Capture(
        const Widget* root = nullptr,
        const OverlayRenderer* overlay = nullptr);
};

} // namespace we::runtime::kindui
