// ==============================================================================
// WindEffects — KindUI — OverlayManager
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/IPopupHost.h"

#include <memory>
#include <vector>

namespace we::runtime::kindui {

class KINDUI_API OverlayHost final : public Widget, public IPopupHost {
public:
    OverlayHost();
    ~OverlayHost() override;

    void SetBaseWidget(const std::shared_ptr<Widget>& baseWidget);

    void ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) override;
    void ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) override;
    void CloseTopPopup() override;
    void CloseAllPopups() override;
    void ExecutePendingCallbacks() override;
    [[nodiscard]] bool HasOpenPopups() const override { return !m_Popups.empty(); }
    [[nodiscard]] bool IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const override;

    /// Floating panels: survive outside-click dismissal and keep a stable size.
    /// Kept off IPopupHost to avoid breaking the cross-DLL vtable ABI.
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

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    [[nodiscard]] std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip = nullptr) override;

private:
    void RemovePopupAt(size_t index);
    [[nodiscard]] int FindPopupIndex(const std::shared_ptr<Widget>& popup) const;

    std::shared_ptr<Widget> m_BaseWidget;
    std::vector<std::shared_ptr<Widget>> m_Popups;
    std::vector<bool> m_FullscreenPopups;
    std::vector<bool> m_PinnedPopups;
    std::vector<Size> m_PopupCachedSizes;
    Size m_LastArrangeSize{};
};

using OverlayManager = OverlayHost;

} // namespace we::runtime::kindui
