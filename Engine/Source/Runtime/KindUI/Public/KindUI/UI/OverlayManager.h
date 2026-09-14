// ==============================================================================
// WindEffects — KindUI — OverlayManager
// Public API surface for the KindUI module.
//
// Canonical floating-UI compositor: dropdowns, context menus, tooltips, combo
// boxes, color/asset pickers, dialogs and pinned panels share one OverlayHost.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/UI/IPopupHost.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {

/// Semantic overlay role (z-order / dismissal / interaction policy).
enum class OverlayKind : uint8_t {
    Transient = 0, ///< Menus, dropdowns — dismissed on outside click.
    Tooltip = 1,   ///< Non-interactive hover hints.
    Pinned = 2,    ///< Floating panels — survive outside click.
    Modal = 3,     ///< Fullscreen/scrim dialogs.
    Fullscreen = 4 ///< Full-viewport overlays (dock previews, etc.).
};

/// Explicit compositor entry: anchor, bounds, z-order, visibility, interaction.
struct KINDUI_API OverlayEntry {
    uint64_t id = 0;
    std::shared_ptr<Widget> widget;
    OverlayKind kind = OverlayKind::Transient;
    int32_t zOrder = 0;
    bool visible = true;
    bool interactive = true;
    bool fullscreen = false;
    bool pinned = false;
    /// Uses PopupPositioner (transient menus/dropdowns/popovers/tooltips).
    bool placed = false;
    Size cachedSize{};
    Rect anchorRect{};
    std::weak_ptr<Widget> liveAnchor;
    PopupPlacementMode placementMode = PopupPlacementMode::BottomPreferred;
    Rect placementViewport{};
    Rect lastPaintBounds{};
};

/// Frame stats for overlay isolation benchmarks / WE_UI_BUILD_PROFILE.
struct KINDUI_API OverlayCompositorStats {
    uint32_t openCount = 0;
    uint32_t overlayOnlyLayouts = 0;
    uint32_t fullHostArranges = 0;
    uint32_t slotsMeasured = 0;
    uint32_t slotsArranged = 0;
    uint32_t slotsPainted = 0;
    uint32_t baseArrangesSkipped = 0;
    uint32_t repositionSkipped = 0;
};

class KINDUI_API OverlayHost final : public Widget, public IPopupHost {
public:
    OverlayHost();
    ~OverlayHost() override;

    void SetBaseWidget(const std::shared_ptr<Widget>& baseWidget);
    [[nodiscard]] std::shared_ptr<Widget> GetBaseWidget() const { return m_BaseWidget; }

    void ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) override;
    void ShowAnchoredPopup(
        const std::shared_ptr<Widget>& popup,
        const Rect& anchorRect,
        PopupPlacementMode placementMode = PopupPlacementMode::BottomPreferred) override;
    void ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) override;
    void CloseTopPopup() override;
    void CloseAllPopups() override;
    void ExecutePendingCallbacks() override;
    [[nodiscard]] bool HasOpenPopups() const override { return !m_Entries.empty(); }
    [[nodiscard]] bool IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const override;

    /// Live-widget anchor: remeasures each Arrange when the anchor moves/scrolls/resizes.
    void ShowAnchoredPopup(
        const std::shared_ptr<Widget>& popup,
        const std::shared_ptr<Widget>& anchorWidget,
        PopupPlacementMode placementMode = PopupPlacementMode::BottomPreferred);

    /// Non-interactive tooltip overlay (high z-order, transient dismissal).
    void ShowTooltip(
        const std::shared_ptr<Widget>& tooltip,
        const Rect& anchorRect,
        PopupPlacementMode placementMode = PopupPlacementMode::SidePreferred);
    void ShowTooltip(
        const std::shared_ptr<Widget>& tooltip,
        const std::shared_ptr<Widget>& anchorWidget,
        PopupPlacementMode placementMode = PopupPlacementMode::SidePreferred);
    void CloseTooltips();

    /// Modal / dialog content via the same compositor (fullscreen + pinned).
    void ShowModal(const std::shared_ptr<Widget>& modal);

    void ShowPinnedPopup(
        const std::shared_ptr<Widget>& popup,
        const Point& position,
        const Size& preferredSize);
    void ShowPinnedFullscreenPopup(const std::shared_ptr<Widget>& popup);
    void MovePopup(const std::shared_ptr<Widget>& popup, const Point& position);
    void ResizePopup(const std::shared_ptr<Widget>& popup, const Rect& bounds);
    void ClosePopup(const std::shared_ptr<Widget>& popup);
    void CloseTransientPopups();
    [[nodiscard]] bool IsPinnedPopup(const std::shared_ptr<Widget>& popup) const;

    /// Overlay-only Measure/Arrange — base editor tree is left untouched.
    void SyncOverlaysOnly();
    /// Same as SyncOverlaysOnly, but seeds host viewport first (WeLauncher embeds OverlayHost
    /// outside the normal child tree and may not have arranged it yet).
    void SyncOverlaysOnly(const Rect& hostViewport);

    [[nodiscard]] const std::vector<OverlayEntry>& Entries() const { return m_Entries; }
    [[nodiscard]] static OverlayCompositorStats& Stats();
    static void ResetStats();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip = nullptr) override;

private:
    void RemoveEntryAt(size_t index);
    [[nodiscard]] int FindEntryIndex(const std::shared_ptr<Widget>& popup) const;
    void PlaceTransientEntry(OverlayEntry& entry, bool forceRemeasure);
    void LayoutOverlayEntries(bool hostResized);
    void AttachOverlayEntry(OverlayEntry& entry);
    void MarkOverlayOpenCloseDirty(const Rect& bounds);
    void RequestOverlayUpdate(const char* reason);
    [[nodiscard]] Rect ResolveLiveAnchorRect(const OverlayEntry& entry) const;
    [[nodiscard]] Rect ResolvePlacementViewport(const std::shared_ptr<Widget>& anchorWidget) const;
    [[nodiscard]] static bool RectsNearlyEqual(const Rect& a, const Rect& b, float eps = 0.5f);
    [[nodiscard]] static int32_t DefaultZOrder(OverlayKind kind);
    uint64_t NextEntryId();

    std::shared_ptr<Widget> m_BaseWidget;
    std::vector<OverlayEntry> m_Entries;
    Size m_LastArrangeSize{};
    uint64_t m_NextEntryId = 1;
    bool m_OverlaysNeedLayout = false;
};

using OverlayManager = OverlayHost;

} // namespace we::runtime::kindui
