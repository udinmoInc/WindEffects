// ==============================================================================
// WindEffects — KindUI — UIDirtyRegionTracker
// Global dirty-rectangle tracking for KindUI. Invalidation feeds regions here;
// the shared paint/drawgen path consumes a merged set each rebuild. Future panels
// inherit this automatically — no per-panel dirty hacks.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/PaintContext.h"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>

namespace we::runtime::kindui {

/// Compact AABB dirty region in root/overlay space (logical pixels).
struct KINDUI_API DirtyRegion {
    Rect rect{};

    [[nodiscard]] float Area() const noexcept {
        return (std::max)(0.0f, rect.width) * (std::max)(0.0f, rect.height);
    }
    [[nodiscard]] bool Empty() const noexcept { return rect.IsEmpty(); }
};

struct KINDUI_API DirtyRegionFrameStats {
    uint32_t regionCount = 0;
    uint32_t mergedCount = 0;
    uint32_t rawAddCount = 0;
    float dirtyAreaPx = 0.0f;
    float viewportAreaPx = 0.0f;
    float coverage = 0.0f; // dirtyArea / viewport, clamped 0..1
    bool fullDirty = false;
    bool geometryReused = false; // drawgen skipped via command hash
    uint64_t commandContentHash = 0;
};

/// Process-wide dirty region accumulator. Thread-safe Add*; Consume* on UI thread.
class KINDUI_API UIDirtyRegionTracker {
public:
    static UIDirtyRegionTracker& Get();

    /// Queue a dirty rect (inflated slightly for outlines/shadows). Empty ignored.
    void Add(const Rect& rect, float inflatePx = 2.0f);
    /// Geometry move: old and new bounds.
    void AddMove(const Rect& previous, const Rect& next, float inflatePx = 2.0f);
    /// Force full-frame dirty (resize, first frames, structural layout).
    void MarkFullDirty();

    [[nodiscard]] bool IsFullDirty() const;
    [[nodiscard]] uint32_t PendingRawCount() const;

    /// Merge pending rects into a compact list for this paint. Clears pending.
    /// `viewport` used for coverage + full-dirty clamp.
    DirtyRegionFrameStats ConsumeMerged(const Rect& viewport);

    /// Last consumed stats (for profiling after ProcessWidget).
    [[nodiscard]] const DirtyRegionFrameStats& LastStats() const { return m_LastStats; }
    [[nodiscard]] const std::vector<Rect>& LastRegions() const { return m_LastRegions; }
    void SetGeometryReused(bool reused) { m_LastStats.geometryReused = reused; }
    void SetCommandContentHash(uint64_t hash) { m_LastStats.commandContentHash = hash; }

    void Reset();

    /// Max retained regions after merge (excess → full dirty).
    static constexpr size_t kMaxRegions = 16;
    /// If coverage ≥ this after merge, treat as full dirty.
    static constexpr float kFullCoverageThreshold = 0.85f;

private:
    UIDirtyRegionTracker() = default;

    static Rect Inflate(const Rect& r, float px);
    static Rect UnionRect(const Rect& a, const Rect& b);
    static bool IntersectsOrTouches(const Rect& a, const Rect& b, float pad);
    void MergeInPlace(std::vector<Rect>& regions) const;

    mutable std::mutex m_Mutex;
    std::vector<Rect> m_Pending;
    bool m_FullDirty = false;
    uint32_t m_RawAddCount = 0;
    DirtyRegionFrameStats m_LastStats{};
    std::vector<Rect> m_LastRegions;
};

/// Hash over draw commands (geometry-relevant fields only).
[[nodiscard]] KINDUI_API uint64_t HashDrawCommands(const std::vector<DrawCommand>& commands);

} // namespace we::runtime::kindui
