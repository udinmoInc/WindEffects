// ==============================================================================
// WindEffects — KindUI — KindUIHeapStats
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIHeapStats.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/UIResourceResidency.h"
#include "KindUI/Host/OverlayRenderer.h"

#include "Platform/Platform.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace we::runtime::kindui {
namespace {

std::atomic<uint64_t> g_AllocCount{0};
std::atomic<uint64_t> g_FreeCount{0};
std::atomic<uint64_t> g_Outstanding{0};
std::atomic<uint64_t> g_PeakOutstanding{0};
std::atomic<int> g_Enabled{-1}; // -1 unknown, 0 off, 1 on

bool EnvOn(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

void CountWidgets(
    const Widget* w,
    uint32_t& total,
    uint32_t& visible,
    uint64_t& retainedBytes,
    std::vector<const void*>& seenStores) {
    if (!w) {
        return;
    }
    ++total;
    if (w->IsVisible()) {
        ++visible;
    }
    if (const void* store = w->RetainedPaintStorePtr()) {
        bool seen = false;
        for (const void* p : seenStores) {
            if (p == store) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            seenStores.push_back(store);
            retainedBytes += w->EstimateRetainedPaintCapacityBytes();
        }
    }
    for (const auto& child : w->GetChildren()) {
        CountWidgets(child.get(), total, visible, retainedBytes, seenStores);
    }
}

void FillProcessMemory(KindUIMemorySnapshot& snap) {
    // Prefer Platform (safe). Avoid hand-rolled K32GetProcessMemoryInfo layouts —
    // packing mistakes here previously correlated with STATUS_HEAP_CORRUPTION reports.
    const auto info = we::platform::Platform::Get().GetMemoryInfo();
    snap.processWorkingSetBytes =
        info.processWorkingSetBytes != 0 ? info.processWorkingSetBytes : info.processUsedBytes;
    snap.processPrivateBytes =
        info.processPrivateBytes != 0 ? info.processPrivateBytes : snap.processWorkingSetBytes;
    snap.processPeakWorkingSetBytes =
        info.processPeakWorkingSetBytes != 0 ? info.processPeakWorkingSetBytes : snap.processWorkingSetBytes;
}

} // namespace

bool KindUIHeapStats::IsTrackingEnabled() {
#if !defined(WE_KINDUI_HEAP_TRACK) || WE_KINDUI_HEAP_TRACK == 0
    return false;
#else
    int enabled = g_Enabled.load(std::memory_order_relaxed);
    if (enabled < 0) {
        RefreshEnabledFromEnv();
        enabled = g_Enabled.load(std::memory_order_relaxed);
    }
    return enabled > 0;
#endif
}

void KindUIHeapStats::RefreshEnabledFromEnv() {
#if defined(WE_KINDUI_HEAP_TRACK) && WE_KINDUI_HEAP_TRACK
    g_Enabled.store(EnvOn("WE_KINDUI_HEAP") ? 1 : 0, std::memory_order_relaxed);
#else
    g_Enabled.store(0, std::memory_order_relaxed);
#endif
}

uint64_t KindUIHeapStats::AllocCount() {
    return g_AllocCount.load(std::memory_order_relaxed);
}
uint64_t KindUIHeapStats::FreeCount() {
    return g_FreeCount.load(std::memory_order_relaxed);
}
uint64_t KindUIHeapStats::OutstandingBytes() {
    return g_Outstanding.load(std::memory_order_relaxed);
}
uint64_t KindUIHeapStats::PeakOutstandingBytes() {
    return g_PeakOutstanding.load(std::memory_order_relaxed);
}

void KindUIHeapStats::ResetPeaks() {
    g_PeakOutstanding.store(g_Outstanding.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void KindUIHeapStats::OnAlloc(size_t bytes) {
    g_AllocCount.fetch_add(1, std::memory_order_relaxed);
    const uint64_t out = g_Outstanding.fetch_add(static_cast<uint64_t>(bytes), std::memory_order_relaxed)
        + static_cast<uint64_t>(bytes);
    uint64_t peak = g_PeakOutstanding.load(std::memory_order_relaxed);
    while (out > peak && !g_PeakOutstanding.compare_exchange_weak(peak, out, std::memory_order_relaxed)) {
    }
}

void KindUIHeapStats::OnFree(size_t bytes) {
    g_FreeCount.fetch_add(1, std::memory_order_relaxed);
    g_Outstanding.fetch_sub(static_cast<uint64_t>(bytes), std::memory_order_relaxed);
}

KindUIMemorySnapshot KindUIHeapStats::Capture(const Widget* root, const OverlayRenderer* overlay) {
    KindUIMemorySnapshot snap{};
    FillProcessMemory(snap);

    snap.kinduiHeapTracking = IsTrackingEnabled();
    if (snap.kinduiHeapTracking) {
        snap.kinduiAllocCount = AllocCount();
        snap.kinduiFreeCount = FreeCount();
        snap.kinduiOutstandingBytes = OutstandingBytes();
        snap.kinduiPeakOutstandingBytes = PeakOutstandingBytes();
    }

    if (root) {
        std::vector<const void*> seenStores;
        seenStores.reserve(256);
        CountWidgets(root, snap.widgetCount, snap.visibleWidgetCount, snap.retainedPaintBytesEstimate, seenStores);
        snap.widgetObjectBytesEstimate =
            static_cast<uint64_t>(snap.widgetCount) * static_cast<uint64_t>(sizeof(Widget));
    }

    if (overlay) {
        snap.geometryVertexCapacityBytes = overlay->GetGeometryVertexCapacityBytes();
        snap.geometryIndexCapacityBytes = overlay->GetGeometryIndexCapacityBytes();
        snap.geometryBatchCapacityBytes = overlay->GetGeometryBatchCapacityBytes();
        snap.drawCommandCapacityBytes = overlay->GetDrawCommandCapacityBytes();
        snap.gpuVertexCapacityBytes = overlay->GetGpuVertexCapacityBytes();
        snap.gpuIndexCapacityBytes = overlay->GetGpuIndexCapacityBytes();
        snap.textMeasureCacheEntries = static_cast<uint32_t>(overlay->GetTextMeasureCacheEntryCount());
        snap.textMeasureCacheBytes = overlay->GetTextMeasureCacheBytes();
        snap.fontAtlasPageCount = overlay->GetFontAtlasPageCount();
        snap.fontAtlasCpuBytes = overlay->GetFontAtlasCpuBytes();
        snap.iconTextureCacheEntries = static_cast<uint32_t>(overlay->GetIconTextureCacheEntryCount());
        snap.iconTextureCacheBytes = overlay->GetIconTextureCacheBytes();
        snap.submissionCacheSlots = overlay->GetSubmissionCacheSlotCount();
    }

    snap.textMetricsCacheEntries = static_cast<uint32_t>(TextMetrics::CacheEntryCount());
    snap.textMetricsCacheBytes = TextMetrics::EstimateCacheBytes();

    {
        const auto& rs = UIResourceResidency::Get().Stats();
        snap.residencyIconCount = rs.residentIconCount;
        snap.residencyIconGpuBytes = rs.residentIconGpuBytes;
        snap.residencyTextGeomCount = rs.residentTextGeomCount;
        snap.residencyTextGeomBytes = rs.residentTextGeomCpuBytes;
        snap.residencyGlyphCount = rs.residentGlyphCount;
        snap.residencyLoads = rs.loads;
        snap.residencyUploads = rs.uploads;
        snap.residencyEvictions = rs.evictions;
        snap.residencyCacheHits = rs.cacheHits;
        snap.residencyCacheMisses = rs.cacheMisses;
        snap.residencyDeferredReleases = rs.deferredReleases;
    }

    snap.kinduiCpuStructuralBytes =
        snap.widgetObjectBytesEstimate
        + snap.retainedPaintBytesEstimate
        + snap.geometryVertexCapacityBytes
        + snap.geometryIndexCapacityBytes
        + snap.geometryBatchCapacityBytes
        + snap.drawCommandCapacityBytes
        + snap.textMeasureCacheBytes
        + snap.textMetricsCacheBytes
        + snap.fontAtlasCpuBytes
        + snap.iconTextureCacheBytes;
    return snap;
}

} // namespace we::runtime::kindui

// NOTE: Do NOT install global operator new/delete in this DLL.
// Cross-module CRT / aligned-new mismatches caused STATUS_HEAP_CORRUPTION (0xC0000374).
// Process Working Set / Private Bytes + structural walk remain the supported measurements.
// KindUI-owned heap attribution via module new is intentionally not measured.
