// ==============================================================================
// WindEffects — KindUI — ListVirtualization
// Shared visible-range math and widget recycle helpers for VirtualList / trees.
// Future panels inherit virtualization by using VirtualList or this API — no
// per-panel virtualization hacks.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace we::runtime::kindui {

/// Inclusive visible index window into a flat item list.
struct KINDUI_API ListVisibleRange {
    size_t first = 0;
    size_t count = 0;

    [[nodiscard]] size_t LastExclusive() const noexcept { return first + count; }
    [[nodiscard]] bool Empty() const noexcept { return count == 0; }
    [[nodiscard]] bool Contains(size_t index) const noexcept {
        return index >= first && index < LastExclusive();
    }
};

/// Fixed-height range from scroll offset + viewport. Overscan adds rows above/below.
[[nodiscard]] inline ListVisibleRange ComputeFixedVisibleRange(
    float scrollOffset,
    float viewportHeight,
    size_t itemCount,
    float itemHeight,
    size_t overscan = 4) noexcept
{
    ListVisibleRange range{};
    if (itemCount == 0 || itemHeight <= 0.0f || viewportHeight <= 0.0f) {
        return range;
    }
    const float scroll = (std::max)(0.0f, scrollOffset);
    const size_t rawFirst = static_cast<size_t>(scroll / itemHeight);
    const size_t first = rawFirst > overscan ? rawFirst - overscan : 0;
    const size_t visibleCore = static_cast<size_t>((std::ceil)(viewportHeight / itemHeight)) + 1;
    const size_t needed = visibleCore + overscan * 2;
    const size_t remaining = itemCount > first ? itemCount - first : 0;
    range.first = first;
    range.count = (std::min)(needed, remaining);
    return range;
}

/// Binary search on prefix sums: prefix[i] = sum of heights[0..i).
/// prefix.size() == itemCount + 1, prefix[0] == 0.
[[nodiscard]] inline ListVisibleRange ComputeVariableVisibleRange(
    float scrollOffset,
    float viewportHeight,
    const std::vector<float>& prefixSums,
    size_t overscan = 4) noexcept
{
    ListVisibleRange range{};
    if (prefixSums.size() < 2 || viewportHeight <= 0.0f) {
        return range;
    }
    const size_t itemCount = prefixSums.size() - 1;
    const float scroll = (std::max)(0.0f, scrollOffset);
    const float endY = scroll + viewportHeight;

    auto lowerBound = [&](float y) -> size_t {
        size_t lo = 0;
        size_t hi = itemCount;
        while (lo < hi) {
            const size_t mid = lo + (hi - lo) / 2;
            if (prefixSums[mid + 1] <= y) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    };

    size_t first = lowerBound(scroll);
    size_t last = lowerBound(endY);
    if (last < itemCount) {
        ++last; // include partial row at bottom
    }
    if (first > overscan) {
        first -= overscan;
    } else {
        first = 0;
    }
    last = (std::min)(itemCount, last + overscan);
    if (last <= first) {
        return range;
    }
    range.first = first;
    range.count = last - first;
    return range;
}

/// Content Y of fixed-height row `index`.
[[nodiscard]] inline float FixedRowOffset(size_t index, float itemHeight) noexcept {
    return static_cast<float>(index) * itemHeight;
}

/// Rebuild prefix sums for variable heights. heights[i] must be > 0.
inline void RebuildHeightPrefix(
    const std::vector<float>& heights,
    std::vector<float>& outPrefix) 
{
    outPrefix.resize(heights.size() + 1);
    outPrefix[0] = 0.0f;
    for (size_t i = 0; i < heights.size(); ++i) {
        outPrefix[i + 1] = outPrefix[i] + (std::max)(1.0f, heights[i]);
    }
}

/// Recycle pool: keep active window widgets, return unused to the free list.
class KINDUI_API WidgetRecyclePool {
public:
    struct Slot {
        size_t dataIndex = static_cast<size_t>(-1);
        std::shared_ptr<Widget> widget;
    };

    /// Ensure `slots` covers [range.first, range.first+range.count) by recycling.
    /// `acquire(index, recycled)` must return a widget bound to `index`.
    /// If recycled is non-null, prefer updating it instead of allocating.
    void SyncWindow(
        ListVisibleRange range,
        std::vector<Slot>& slots,
        const std::function<std::shared_ptr<Widget>(size_t index, std::shared_ptr<Widget> recycled)>& acquire);

    void Clear();

    [[nodiscard]] size_t PoolSize() const noexcept { return m_Free.size(); }
    [[nodiscard]] size_t AcquireCount() const noexcept { return m_AcquireCount; }
    [[nodiscard]] size_t RecycleCount() const noexcept { return m_RecycleCount; }
    [[nodiscard]] size_t CreateCount() const noexcept { return m_CreateCount; }

    void ResetCounters() {
        m_AcquireCount = 0;
        m_RecycleCount = 0;
        m_CreateCount = 0;
    }

private:
    std::vector<std::shared_ptr<Widget>> m_Free;
    uint64_t m_AcquireCount = 0;
    uint64_t m_RecycleCount = 0;
    uint64_t m_CreateCount = 0;
};

} // namespace we::runtime::kindui
