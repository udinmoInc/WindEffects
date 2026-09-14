// ==============================================================================
// WindEffects — KindUI — OverlayManager
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/OverlayManager.h"
#include "KindUI/UI/PopupPositioner.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIStateChange.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/ThemeAccess.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {
namespace {

OverlayCompositorStats g_OverlayStats{};

} // namespace

OverlayHost::OverlayHost() = default;
OverlayHost::~OverlayHost() = default;

OverlayCompositorStats& OverlayHost::Stats() {
    return g_OverlayStats;
}

void OverlayHost::ResetStats() {
    g_OverlayStats = {};
}

int32_t OverlayHost::DefaultZOrder(OverlayKind kind) {
    switch (kind) {
    case OverlayKind::Transient: return 100;
    case OverlayKind::Tooltip: return 200;
    case OverlayKind::Pinned: return 50;
    case OverlayKind::Modal: return 300;
    case OverlayKind::Fullscreen: return 250;
    }
    return 100;
}

uint64_t OverlayHost::NextEntryId() {
    return m_NextEntryId++;
}

void OverlayHost::SetBaseWidget(const std::shared_ptr<Widget>& baseWidget) {
    if (m_BaseWidget) {
        RemoveChild(m_BaseWidget);
    }
    m_BaseWidget = baseWidget;
    if (m_BaseWidget) {
        AddChild(m_BaseWidget);
    }
}

int OverlayHost::FindEntryIndex(const std::shared_ptr<Widget>& popup) const {
    if (!popup) {
        return -1;
    }
    for (size_t i = 0; i < m_Entries.size(); ++i) {
        if (m_Entries[i].widget == popup) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void OverlayHost::RequestOverlayUpdate(const char* reason) {
    m_OverlaysNeedLayout = true;
    UIRepaintGate::RequestOverlayLayoutReason(reason);
    UIRepaintGate::RequestPaintReason(reason);
}

void OverlayHost::MarkOverlayOpenCloseDirty(const Rect& bounds) {
    if (bounds.IsEmpty()) {
        UIDirtyRegionTracker::Get().MarkFullDirty();
    } else {
        UIDirtyRegionTracker::Get().Add(bounds, 8.0f);
    }
}

void OverlayHost::AttachOverlayEntry(OverlayEntry& entry) {
    if (!entry.widget) {
        return;
    }
    entry.widget->SetOverlayContent(true);
    AttachOverlayChild(entry.widget);
    entry.lastPaintBounds = entry.widget->GetGeometry();
    MarkOverlayOpenCloseDirty(entry.lastPaintBounds);
    RequestOverlayUpdate("OverlayOpen");
    g_OverlayStats.openCount = static_cast<uint32_t>(m_Entries.size());
}

void OverlayHost::RemoveEntryAt(size_t index) {
    if (index >= m_Entries.size()) {
        return;
    }
    OverlayEntry& entry = m_Entries[index];
    const Rect dirty = entry.lastPaintBounds.IsEmpty() && entry.widget
        ? entry.widget->GetGeometry()
        : entry.lastPaintBounds;
    if (entry.widget) {
        entry.widget->SetOverlayContent(false);
        DetachOverlayChild(entry.widget);
    }
    MarkOverlayOpenCloseDirty(dirty);
    m_Entries.erase(m_Entries.begin() + static_cast<std::ptrdiff_t>(index));
    RequestOverlayUpdate("OverlayClose");
    g_OverlayStats.openCount = static_cast<uint32_t>(m_Entries.size());
}

bool OverlayHost::RectsNearlyEqual(const Rect& a, const Rect& b, float eps) {
    return std::abs(a.x - b.x) <= eps
        && std::abs(a.y - b.y) <= eps
        && std::abs(a.width - b.width) <= eps
        && std::abs(a.height - b.height) <= eps;
}

Rect OverlayHost::ResolveLiveAnchorRect(const OverlayEntry& entry) const {
    if (auto live = entry.liveAnchor.lock()) {
        return live->GetGeometry();
    }
    return entry.anchorRect;
}

Rect OverlayHost::ResolvePlacementViewport(const std::shared_ptr<Widget>& anchorWidget) const {
    Rect viewport = m_Geometry;
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return viewport;
    }
    for (std::shared_ptr<Widget> current = anchorWidget; current; current = current->GetParent()) {
        if (const std::optional<Rect> clip = current->GetHitTestClipRect()) {
            if (!clip->IsEmpty()) {
                viewport = viewport.Intersect(*clip);
            }
        }
    }
    return viewport;
}

void OverlayHost::PlaceTransientEntry(OverlayEntry& entry, bool forceRemeasure) {
    if (!entry.widget || !entry.placed || !entry.visible) {
        return;
    }

    const Rect resolvedAnchor = ResolveLiveAnchorRect(entry);
    std::shared_ptr<Widget> live = entry.liveAnchor.lock();
    Rect viewport = ResolvePlacementViewport(live);

    if (viewport.width < 32.0f || viewport.height < 32.0f) {
        viewport = Rect{ 0.0f, 0.0f, 4096.0f, 4096.0f };
    }

    const bool anchorMoved = !RectsNearlyEqual(resolvedAnchor, entry.anchorRect);
    const bool viewportChanged = !RectsNearlyEqual(viewport, entry.placementViewport);
    const bool needsLayout = forceRemeasure || entry.widget->SubtreeNeedsLayout()
        || entry.cachedSize.width <= 0.0f || entry.cachedSize.height <= 0.0f
        || anchorMoved || viewportChanged;

    if (!needsLayout) {
        const Rect current = entry.widget->GetGeometry();
        if (current.width > 0.0f && current.height > 0.0f) {
            ArrangeChild(entry.widget, current);
            entry.lastPaintBounds = current;
            ++g_OverlayStats.repositionSkipped;
            ++g_OverlayStats.slotsArranged;
            return;
        }
    }

    entry.anchorRect = resolvedAnchor;
    entry.placementViewport = viewport;

    const float margin = ResolveMetric(MetricToken::Space2);
    const float availW = std::max(1.0f, viewport.width - margin * 2.0f);
    const float availH = std::max(1.0f, viewport.height - margin * 2.0f);

    Size size = MeasureChild(entry.widget, Size{ availW, availH });
    size = entry.widget->ClampDesiredSize(size);
    size.width = std::min(size.width, availW);
    size.height = std::min(size.height, availH);
    entry.cachedSize = size;
    ++g_OverlayStats.slotsMeasured;

    PopupPlacementOptions options{};
    options.anchorRect = resolvedAnchor;
    options.mode = entry.placementMode;
    options.gap = ResolveMetric(MetricToken::Space1);
    options.viewportMargin = margin;
    options.viewportBounds = viewport;

    const PopupPlacementResult placement = PopupPositioner::Calculate(size, options);
    const Rect oldBounds = entry.lastPaintBounds;
    ArrangeChild(entry.widget, placement.popupRect);
    entry.cachedSize = Size{ placement.popupRect.width, placement.popupRect.height };
    entry.lastPaintBounds = placement.popupRect;
    if (!oldBounds.IsEmpty()) {
        UIDirtyRegionTracker::Get().Add(oldBounds, 4.0f);
    }
    UIDirtyRegionTracker::Get().Add(placement.popupRect, 4.0f);
    ++g_OverlayStats.slotsArranged;
}

void OverlayHost::LayoutOverlayEntries(bool hostResized) {
    const float margin = ResolveMetric(MetricToken::Space2);

    // Stable paint/hit order by z-order (insertion order as tie-break).
    std::stable_sort(m_Entries.begin(), m_Entries.end(),
        [](const OverlayEntry& a, const OverlayEntry& b) {
            if (a.zOrder != b.zOrder) {
                return a.zOrder < b.zOrder;
            }
            return a.id < b.id;
        });

    for (OverlayEntry& entry : m_Entries) {
        auto& popup = entry.widget;
        if (!popup || !entry.visible) {
            continue;
        }

        if (entry.fullscreen || entry.kind == OverlayKind::Modal
            || entry.kind == OverlayKind::Fullscreen) {
            if (hostResized || popup->SubtreeNeedsLayout()) {
                (void)MeasureChild(popup, Size{ m_Geometry.width, m_Geometry.height });
                ++g_OverlayStats.slotsMeasured;
            }
            ArrangeChild(popup, m_Geometry);
            entry.cachedSize = Size{ m_Geometry.width, m_Geometry.height };
            entry.lastPaintBounds = m_Geometry;
            ++g_OverlayStats.slotsArranged;
            continue;
        }

        if (entry.placed) {
            PlaceTransientEntry(entry, hostResized);
            continue;
        }

        Rect geom = popup->GetGeometry();
        Size size = entry.cachedSize;
        if (size.width <= 0.0f || size.height <= 0.0f) {
            size = Size{ geom.width, geom.height };
        }

        if (entry.pinned) {
            if (popup->SubtreeNeedsLayout()) {
                (void)MeasureChild(popup, size);
                ++g_OverlayStats.slotsMeasured;
            }
        } else if (popup->SubtreeNeedsLayout() || size.width <= 0.0f || size.height <= 0.0f) {
            const float maxW = std::max(1.0f, m_Geometry.width - margin * 2.0f);
            const float availH = std::max(1.0f, m_Geometry.height - geom.y - margin);
            size = MeasureChild(popup, Size{ maxW, availH });
            size = popup->ClampDesiredSize(size);
            ++g_OverlayStats.slotsMeasured;
        }

        PopupPlacementOptions options{};
        options.anchorRect = Rect{ geom.x, geom.y, 0.0f, 0.0f };
        options.mode = PopupPlacementMode::AtPoint;
        options.gap = 0.0f;
        options.viewportMargin = margin;
        options.viewportBounds = m_Geometry;

        const PopupPlacementResult placement = PopupPositioner::Calculate(size, options);
        ArrangeChild(popup, placement.popupRect);
        entry.cachedSize = Size{ placement.popupRect.width, placement.popupRect.height };
        entry.lastPaintBounds = placement.popupRect;
        ++g_OverlayStats.slotsArranged;
    }

    m_OverlaysNeedLayout = false;
}

void OverlayHost::SyncOverlaysOnly() {
    // Overlay-only path: never Arrange the base shell widget.
    if (m_Geometry.width <= 0.0f || m_Geometry.height <= 0.0f) {
        return;
    }
    ++g_OverlayStats.overlayOnlyLayouts;
    LayoutOverlayEntries(false);
    for (OverlayEntry& entry : m_Entries) {
        if (entry.widget) {
            entry.widget->ClearSubtreeLayoutDirty();
        }
    }
}

void OverlayHost::SyncOverlaysOnly(const Rect& hostViewport) {
    if (hostViewport.width <= 0.0f || hostViewport.height <= 0.0f) {
        return;
    }
    const bool hostResized = !RectsNearlyEqual(hostViewport, m_Geometry)
        || std::abs(hostViewport.width - m_LastArrangeSize.width) > 0.5f
        || std::abs(hostViewport.height - m_LastArrangeSize.height) > 0.5f;
    // Seed viewport only — do not CommitGeometry (avoids retained-paint ancestor walks
    // for OverlayHosts that are not in the base widget tree).
    m_Geometry = hostViewport;
    m_LastArrangeSize = Size{ hostViewport.width, hostViewport.height };
    ++g_OverlayStats.overlayOnlyLayouts;
    LayoutOverlayEntries(hostResized);
    for (OverlayEntry& entry : m_Entries) {
        if (entry.widget) {
            entry.widget->ClearSubtreeLayoutDirty();
        }
    }
}

void OverlayHost::ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Transient;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.placed = true;
    entry.interactive = true;
    entry.anchorRect = Rect{ position.x, position.y, 0.0f, 0.0f };
    entry.placementMode = PopupPlacementMode::BottomPreferred;
    popup->SetOverlayContent(true);
    PlaceTransientEntry(entry, true);
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowAnchoredPopup(
    const std::shared_ptr<Widget>& popup,
    const Rect& anchorRect,
    PopupPlacementMode placementMode) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Transient;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.placed = true;
    entry.interactive = true;
    entry.anchorRect = anchorRect;
    entry.placementMode = placementMode;
    popup->SetOverlayContent(true);
    PlaceTransientEntry(entry, true);
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowAnchoredPopup(
    const std::shared_ptr<Widget>& popup,
    const std::shared_ptr<Widget>& anchorWidget,
    PopupPlacementMode placementMode) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Transient;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.placed = true;
    entry.interactive = true;
    entry.placementMode = placementMode;
    if (anchorWidget) {
        entry.liveAnchor = anchorWidget;
        entry.anchorRect = anchorWidget->GetGeometry();
    }
    popup->SetOverlayContent(true);
    PlaceTransientEntry(entry, true);
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowTooltip(
    const std::shared_ptr<Widget>& tooltip,
    const Rect& anchorRect,
    PopupPlacementMode placementMode) {
    if (!tooltip) {
        return;
    }
    CloseTooltips();
    UIStateChangeGate::ScopedTransaction tx("TooltipOpen");
    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = tooltip;
    entry.kind = OverlayKind::Tooltip;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.placed = true;
    entry.interactive = false;
    entry.anchorRect = anchorRect;
    entry.placementMode = placementMode;
    tooltip->SetOverlayContent(true);
    PlaceTransientEntry(entry, true);
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowTooltip(
    const std::shared_ptr<Widget>& tooltip,
    const std::shared_ptr<Widget>& anchorWidget,
    PopupPlacementMode placementMode) {
    if (!tooltip) {
        return;
    }
    CloseTooltips();
    UIStateChangeGate::ScopedTransaction tx("TooltipOpen");
    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = tooltip;
    entry.kind = OverlayKind::Tooltip;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.placed = true;
    entry.interactive = false;
    entry.placementMode = placementMode;
    if (anchorWidget) {
        entry.liveAnchor = anchorWidget;
        entry.anchorRect = anchorWidget->GetGeometry();
    }
    tooltip->SetOverlayContent(true);
    PlaceTransientEntry(entry, true);
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::CloseTooltips() {
    for (int i = static_cast<int>(m_Entries.size()) - 1; i >= 0; --i) {
        if (m_Entries[static_cast<size_t>(i)].kind == OverlayKind::Tooltip) {
            RemoveEntryAt(static_cast<size_t>(i));
        }
    }
}

void OverlayHost::ShowModal(const std::shared_ptr<Widget>& modal) {
    if (!modal) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("ModalOpen");
    const int existing = FindEntryIndex(modal);
    if (existing >= 0) {
        RemoveEntryAt(static_cast<size_t>(existing));
    }
    modal->SetOverlayContent(true);
    const float width = std::max(m_Geometry.width, 1.0f);
    const float height = std::max(m_Geometry.height, 1.0f);
    const Rect geom{ 0.0f, 0.0f, width, height };
    (void)MeasureChild(modal, Size{ width, height });
    ArrangeChild(modal, geom);

    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = modal;
    entry.kind = OverlayKind::Modal;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.fullscreen = true;
    entry.pinned = true;
    entry.interactive = true;
    entry.cachedSize = Size{ width, height };
    entry.lastPaintBounds = geom;
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    const float width = std::max(m_Geometry.width, 1.0f);
    const float height = std::max(m_Geometry.height, 1.0f);
    const Rect geom{ 0.0f, 0.0f, width, height };
    popup->SetOverlayContent(true);
    (void)MeasureChild(popup, Size{ width, height });
    ArrangeChild(popup, geom);

    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Fullscreen;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.fullscreen = true;
    entry.interactive = true;
    entry.cachedSize = Size{ width, height };
    entry.lastPaintBounds = geom;
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowPinnedPopup(
    const std::shared_ptr<Widget>& popup,
    const Point& position,
    const Size& preferredSize) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    const int existing = FindEntryIndex(popup);
    if (existing >= 0) {
        RemoveEntryAt(static_cast<size_t>(existing));
    }

    const float screenW = std::max(m_Geometry.width, 1.0f);
    const float screenH = std::max(m_Geometry.height, 1.0f);
    const float margin = ResolveMetric(MetricToken::Space2);

    Size size{
        std::max(preferredSize.width, 240.0f),
        std::max(preferredSize.height, 180.0f)
    };
    size.width = std::min(size.width, std::max(240.0f, screenW - margin * 2.0f));
    size.height = std::min(size.height, std::max(180.0f, screenH - margin * 2.0f));

    popup->SetOverlayContent(true);
    (void)MeasureChild(popup, size);
    size = popup->ClampDesiredSize(size);

    PopupPlacementOptions options{};
    options.anchorRect = Rect{ position.x, position.y, 0.0f, 0.0f };
    options.mode = PopupPlacementMode::AtPoint;
    options.gap = 0.0f;
    options.viewportMargin = margin;
    options.viewportBounds = Rect{ 0.0f, 0.0f, screenW, screenH };

    const PopupPlacementResult placement = PopupPositioner::Calculate(size, options);
    ArrangeChild(popup, placement.popupRect);

    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Pinned;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.pinned = true;
    entry.interactive = true;
    entry.cachedSize = Size{ placement.popupRect.width, placement.popupRect.height };
    entry.lastPaintBounds = placement.popupRect;
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::ShowPinnedFullscreenPopup(const std::shared_ptr<Widget>& popup) {
    if (!popup) {
        return;
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayOpen");
    const int existing = FindEntryIndex(popup);
    if (existing >= 0) {
        RemoveEntryAt(static_cast<size_t>(existing));
    }

    const float width = std::max(m_Geometry.width, 1.0f);
    const float height = std::max(m_Geometry.height, 1.0f);
    const Rect geom{ 0.0f, 0.0f, width, height };
    popup->SetOverlayContent(true);
    (void)MeasureChild(popup, Size{ width, height });
    ArrangeChild(popup, geom);

    OverlayEntry entry{};
    entry.id = NextEntryId();
    entry.widget = popup;
    entry.kind = OverlayKind::Fullscreen;
    entry.zOrder = DefaultZOrder(entry.kind);
    entry.fullscreen = true;
    entry.pinned = true;
    entry.interactive = true;
    entry.cachedSize = Size{ width, height };
    entry.lastPaintBounds = geom;
    m_Entries.push_back(std::move(entry));
    AttachOverlayEntry(m_Entries.back());
}

void OverlayHost::MovePopup(const std::shared_ptr<Widget>& popup, const Point& position) {
    const int index = FindEntryIndex(popup);
    if (index < 0) {
        return;
    }

    OverlayEntry& entry = m_Entries[static_cast<size_t>(index)];
    if (entry.fullscreen) {
        return;
    }

    const float screenW = std::max(m_Geometry.width, 1.0f);
    const float screenH = std::max(m_Geometry.height, 1.0f);
    const float margin = ResolveMetric(MetricToken::Space2);

    Size size = entry.cachedSize;
    if (size.width <= 0.0f || size.height <= 0.0f) {
        const Rect current = popup->GetGeometry();
        size = Size{ current.width, current.height };
    }

    PopupPlacementOptions options{};
    options.anchorRect = Rect{ position.x, position.y, 0.0f, 0.0f };
    options.mode = PopupPlacementMode::AtPoint;
    options.gap = 0.0f;
    options.viewportMargin = margin;
    options.viewportBounds = Rect{ 0.0f, 0.0f, screenW, screenH };

    const Rect oldBounds = entry.lastPaintBounds;
    const PopupPlacementResult placement = PopupPositioner::Calculate(size, options);
    ArrangeChild(popup, placement.popupRect);
    entry.cachedSize = Size{ placement.popupRect.width, placement.popupRect.height };
    entry.lastPaintBounds = placement.popupRect;
    if (!oldBounds.IsEmpty()) {
        UIDirtyRegionTracker::Get().Add(oldBounds, 4.0f);
    }
    UIDirtyRegionTracker::Get().Add(placement.popupRect, 4.0f);
    UIRepaintGate::RequestPaintReason("OverlayMove");
}

void OverlayHost::ResizePopup(const std::shared_ptr<Widget>& popup, const Rect& bounds) {
    const int index = FindEntryIndex(popup);
    if (index < 0) {
        return;
    }

    OverlayEntry& entry = m_Entries[static_cast<size_t>(index)];
    if (entry.fullscreen) {
        return;
    }

    const float screenW = std::max(m_Geometry.width, 1.0f);
    const float screenH = std::max(m_Geometry.height, 1.0f);
    const float margin = ResolveMetric(MetricToken::Space2);

    Size size{
        std::max(120.0f, bounds.width),
        std::max(28.0f, bounds.height)
    };
    size.width = std::min(size.width, std::max(120.0f, screenW - margin * 2.0f));
    size.height = std::min(size.height, std::max(28.0f, screenH - margin * 2.0f));

    PopupPlacementOptions options{};
    options.anchorRect = Rect{ bounds.x, bounds.y, 0.0f, 0.0f };
    options.mode = PopupPlacementMode::AtPoint;
    options.gap = 0.0f;
    options.viewportMargin = margin;
    options.viewportBounds = Rect{ 0.0f, 0.0f, screenW, screenH };

    (void)MeasureChild(popup, size);
    const Rect oldBounds = entry.lastPaintBounds;
    const PopupPlacementResult placement = PopupPositioner::Calculate(size, options);
    ArrangeChild(popup, placement.popupRect);
    entry.cachedSize = Size{ placement.popupRect.width, placement.popupRect.height };
    entry.lastPaintBounds = placement.popupRect;
    if (!oldBounds.IsEmpty()) {
        UIDirtyRegionTracker::Get().Add(oldBounds, 4.0f);
    }
    UIDirtyRegionTracker::Get().Add(placement.popupRect, 4.0f);
    RequestOverlayUpdate("OverlayResize");
}

void OverlayHost::ClosePopup(const std::shared_ptr<Widget>& popup) {
    const int index = FindEntryIndex(popup);
    if (index >= 0) {
        UIStateChangeGate::ScopedTransaction tx("OverlayClose");
        RemoveEntryAt(static_cast<size_t>(index));
    }
}

void OverlayHost::CloseTopPopup() {
    if (m_Entries.empty()) {
        return;
    }
    // Close topmost interactive/transient (highest z), skip pinned unless only entry.
    for (int i = static_cast<int>(m_Entries.size()) - 1; i >= 0; --i) {
        const OverlayEntry& e = m_Entries[static_cast<size_t>(i)];
        if (!e.pinned || e.kind == OverlayKind::Tooltip || e.kind == OverlayKind::Transient) {
            UIStateChangeGate::ScopedTransaction tx("OverlayClose");
            RemoveEntryAt(static_cast<size_t>(i));
            return;
        }
    }
    UIStateChangeGate::ScopedTransaction tx("OverlayClose");
    RemoveEntryAt(m_Entries.size() - 1);
}

void OverlayHost::CloseTransientPopups() {
    UIStateChangeGate::ScopedTransaction tx("OverlayClose");
    for (int i = static_cast<int>(m_Entries.size()) - 1; i >= 0; --i) {
        const OverlayEntry& e = m_Entries[static_cast<size_t>(i)];
        if (!e.pinned) {
            RemoveEntryAt(static_cast<size_t>(i));
        }
    }
}

void OverlayHost::CloseAllPopups() {
    UIStateChangeGate::ScopedTransaction tx("OverlayClose");
    for (auto& entry : m_Entries) {
        if (entry.widget) {
            entry.widget->SetOverlayContent(false);
            DetachOverlayChild(entry.widget);
        }
    }
    m_Entries.clear();
    UIDirtyRegionTracker::Get().MarkFullDirty();
    RequestOverlayUpdate("OverlayCloseAll");
    g_OverlayStats.openCount = 0;
}

void OverlayHost::ExecutePendingCallbacks() {
    std::vector<std::shared_ptr<Widget>> snapshot;
    snapshot.reserve(m_Entries.size());
    for (const auto& entry : m_Entries) {
        snapshot.push_back(entry.widget);
    }
    for (const auto& popup : snapshot) {
        if (popup) {
            popup->ExecutePendingCallback();
        }
    }
}

bool OverlayHost::IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const {
    if (!widget) {
        return false;
    }
    std::shared_ptr<Widget> current = widget;
    for (int depth = 0; current && depth < 256; ++depth) {
        for (const auto& entry : m_Entries) {
            if (entry.widget && current == entry.widget) {
                return true;
            }
        }
        current = current->GetParent();
    }
    return false;
}

bool OverlayHost::IsPinnedPopup(const std::shared_ptr<Widget>& popup) const {
    const int index = FindEntryIndex(popup);
    if (index < 0) {
        return false;
    }
    return m_Entries[static_cast<size_t>(index)].pinned;
}

Size OverlayHost::Measure(const Size& availableSize) {
    if (CanSkipMeasure(availableSize)) {
        return m_DesiredSize;
    }
    m_DesiredSize = availableSize;
    if (m_BaseWidget) {
        (void)MeasureChild(m_BaseWidget, availableSize);
    }
    NoteMeasureCache(availableSize);
    return availableSize;
}

void OverlayHost::Arrange(const Rect& allottedRect) {
    CommitGeometry(allottedRect);
    const bool hostResized =
        std::abs(allottedRect.width - m_LastArrangeSize.width) > 0.5f
        || std::abs(allottedRect.height - m_LastArrangeSize.height) > 0.5f;
    m_LastArrangeSize = Size{ allottedRect.width, allottedRect.height };
    ClearLayoutDirty();
    ++g_OverlayStats.fullHostArranges;

    const bool baseNeedsArrange = hostResized
        || (m_BaseWidget && m_BaseWidget->SubtreeNeedsLayout())
        || (m_BaseWidget && !RectsNearlyEqual(m_BaseWidget->GetGeometry(), allottedRect));

    if (m_BaseWidget && baseNeedsArrange) {
        ArrangeChild(m_BaseWidget, allottedRect);
    } else if (m_BaseWidget) {
        ++g_OverlayStats.baseArrangesSkipped;
    }

    LayoutOverlayEntries(hostResized);
}

void OverlayHost::Paint(PaintContext& context) {
    if (m_BaseWidget) {
        m_BaseWidget->PaintSubtree(context);
    }
    for (auto& entry : m_Entries) {
        if (entry.widget && entry.visible) {
            entry.widget->PaintSubtree(context);
            entry.lastPaintBounds = entry.widget->GetGeometry();
            ++g_OverlayStats.slotsPainted;
        }
    }
}

void OverlayHost::OnMouseDown(const MouseEvent&) {
    // Dismissal is handled by EventSystem when clicking outside a popup.
}

std::shared_ptr<Widget> OverlayHost::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }

    // Highest z-order first (entries sorted ascending in LayoutOverlayEntries).
    for (auto it = m_Entries.rbegin(); it != m_Entries.rend(); ++it) {
        if (!it->widget || !it->visible || !it->interactive) {
            continue;
        }
        if (auto hit = it->widget->HitTestPoint(pos, clip)) {
            return hit;
        }
    }

    if (m_BaseWidget) {
        return m_BaseWidget->HitTestPoint(pos, clip);
    }

    return nullptr;
}

} // namespace we::runtime::kindui
