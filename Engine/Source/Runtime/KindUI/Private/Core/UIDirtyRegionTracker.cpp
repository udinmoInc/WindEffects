// ==============================================================================
// WindEffects — KindUI — UIDirtyRegionTracker
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/PaintContext.h"

#include <algorithm>
#include <cstring>

namespace we::runtime::kindui {

namespace {

void HashMix(uint64_t& h, uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
}

void HashFloat(uint64_t& h, float f) {
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(f));
    std::memcpy(&bits, &f, sizeof(bits));
    HashMix(h, bits);
}

void HashRect(uint64_t& h, const Rect& r) {
    HashFloat(h, r.x);
    HashFloat(h, r.y);
    HashFloat(h, r.width);
    HashFloat(h, r.height);
}

void HashColor(uint64_t& h, const Color& c) {
    HashFloat(h, c.r);
    HashFloat(h, c.g);
    HashFloat(h, c.b);
    HashFloat(h, c.a);
}

} // namespace

UIDirtyRegionTracker& UIDirtyRegionTracker::Get() {
    static UIDirtyRegionTracker instance;
    return instance;
}

Rect UIDirtyRegionTracker::Inflate(const Rect& r, float px) {
    if (r.IsEmpty()) {
        return {};
    }
    return Rect{
        r.x - px,
        r.y - px,
        r.width + px * 2.0f,
        r.height + px * 2.0f};
}

Rect UIDirtyRegionTracker::UnionRect(const Rect& a, const Rect& b) {
    if (a.IsEmpty()) {
        return b;
    }
    if (b.IsEmpty()) {
        return a;
    }
    const float x0 = (std::min)(a.x, b.x);
    const float y0 = (std::min)(a.y, b.y);
    const float x1 = (std::max)(a.x + a.width, b.x + b.width);
    const float y1 = (std::max)(a.y + a.height, b.y + b.height);
    return Rect{ x0, y0, x1 - x0, y1 - y0 };
}

bool UIDirtyRegionTracker::IntersectsOrTouches(const Rect& a, const Rect& b, float pad) {
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    const Rect aa = Inflate(a, pad);
    const Rect bb = Inflate(b, pad);
    return !aa.Intersect(bb).IsEmpty();
}

void UIDirtyRegionTracker::MergeInPlace(std::vector<Rect>& regions) const {
    if (regions.size() <= 1) {
        return;
    }
    bool merged = true;
    while (merged && regions.size() > 1) {
        merged = false;
        for (size_t i = 0; i < regions.size(); ++i) {
            for (size_t j = i + 1; j < regions.size(); ++j) {
                if (IntersectsOrTouches(regions[i], regions[j], 1.0f)) {
                    regions[i] = UnionRect(regions[i], regions[j]);
                    regions[j] = regions.back();
                    regions.pop_back();
                    merged = true;
                    break;
                }
            }
            if (merged) {
                break;
            }
        }
    }
}

void UIDirtyRegionTracker::Add(const Rect& rect, float inflatePx) {
    if (rect.IsEmpty()) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    if (m_FullDirty) {
        ++m_RawAddCount;
        return;
    }
    Rect r = Inflate(rect, inflatePx);
    if (r.width < 1.0f) {
        r.width = 1.0f;
    }
    if (r.height < 1.0f) {
        r.height = 1.0f;
    }
    m_Pending.push_back(r);
    ++m_RawAddCount;
    // Cap pending growth — pathological spam collapses to full dirty.
    if (m_Pending.size() > kMaxRegions * 4) {
        m_FullDirty = true;
        m_Pending.clear();
    }
}

void UIDirtyRegionTracker::AddMove(const Rect& previous, const Rect& next, float inflatePx) {
    Add(previous, inflatePx);
    Add(next, inflatePx);
}

void UIDirtyRegionTracker::MarkFullDirty() {
    std::lock_guard lock(m_Mutex);
    m_FullDirty = true;
    m_Pending.clear();
    ++m_RawAddCount;
}

bool UIDirtyRegionTracker::IsFullDirty() const {
    std::lock_guard lock(m_Mutex);
    return m_FullDirty;
}

uint32_t UIDirtyRegionTracker::PendingRawCount() const {
    std::lock_guard lock(m_Mutex);
    return m_RawAddCount;
}

DirtyRegionFrameStats UIDirtyRegionTracker::ConsumeMerged(const Rect& viewport) {
    std::lock_guard lock(m_Mutex);
    DirtyRegionFrameStats stats{};
    stats.rawAddCount = m_RawAddCount;
    stats.viewportAreaPx = (std::max)(0.0f, viewport.width) * (std::max)(0.0f, viewport.height);

    std::vector<Rect> regions;
    if (m_FullDirty || viewport.IsEmpty()) {
        stats.fullDirty = true;
        stats.regionCount = 1;
        stats.dirtyAreaPx = stats.viewportAreaPx;
        stats.coverage = 1.0f;
    } else {
        regions.swap(m_Pending);
        const uint32_t before = static_cast<uint32_t>(regions.size());
        MergeInPlace(regions);
        stats.mergedCount = before > regions.size()
            ? before - static_cast<uint32_t>(regions.size())
            : 0;

        if (regions.size() > kMaxRegions) {
            stats.fullDirty = true;
            stats.regionCount = 1;
            stats.dirtyAreaPx = stats.viewportAreaPx;
            stats.coverage = 1.0f;
        } else {
            float area = 0.0f;
            for (const Rect& r : regions) {
                const Rect clipped = r.Intersect(viewport);
                area += clipped.width * clipped.height;
            }
            stats.regionCount = static_cast<uint32_t>(regions.size());
            stats.dirtyAreaPx = area;
            stats.coverage = stats.viewportAreaPx > 0.0f
                ? (std::min)(1.0f, area / stats.viewportAreaPx)
                : 1.0f;
            if (regions.empty() || stats.coverage >= kFullCoverageThreshold) {
                // No concrete rects (or nearly full) → full-frame rebuild.
                stats.fullDirty = true;
                stats.regionCount = 1;
                stats.dirtyAreaPx = stats.viewportAreaPx;
                stats.coverage = 1.0f;
            }
        }
    }

    m_Pending.clear();
    m_FullDirty = false;
    m_RawAddCount = 0;
    m_LastStats = stats;
    if (stats.fullDirty) {
        m_LastRegions.clear();
        if (!viewport.IsEmpty()) {
            m_LastRegions.push_back(viewport);
        }
    } else {
        m_LastRegions = std::move(regions);
    }
    return m_LastStats;
}

void UIDirtyRegionTracker::Reset() {
    std::lock_guard lock(m_Mutex);
    m_Pending.clear();
    m_LastRegions.clear();
    m_FullDirty = false;
    m_RawAddCount = 0;
    m_LastStats = {};
}

uint64_t HashDrawCommands(const std::vector<DrawCommand>& commands) {
    uint64_t h = 14695981039346656037ull;
    HashMix(h, static_cast<uint64_t>(commands.size()));
    for (const DrawCommand& cmd : commands) {
        HashMix(h, static_cast<uint64_t>(cmd.type));
        HashRect(h, cmd.rect);
        HashRect(h, cmd.clipRect);
        HashColor(h, cmd.color);
        HashColor(h, cmd.colorBottom);
        HashFloat(h, cmd.borderRadius);
        HashFloat(h, cmd.thickness);
        HashFloat(h, cmd.blur);
        HashFloat(h, cmd.fontSize);
        HashMix(h, cmd.textWeight);
        HashMix(h, cmd.iconSizePx);
        HashMix(h, static_cast<uint64_t>(cmd.textureId));
        HashMix(h, static_cast<uint64_t>(cmd.text.size()));
        for (unsigned char ch : cmd.text) {
            HashMix(h, ch);
        }
        HashMix(h, static_cast<uint64_t>(cmd.iconStem.size()));
        for (unsigned char ch : cmd.iconStem) {
            HashMix(h, ch);
        }
        HashFloat(h, cmd.lineStart.x);
        HashFloat(h, cmd.lineStart.y);
        HashFloat(h, cmd.lineEnd.x);
        HashFloat(h, cmd.lineEnd.y);
    }
    return h;
}

} // namespace we::runtime::kindui
