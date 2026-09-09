// ==============================================================================
// WindEffects — KindUI — EventSystem
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Input/InputEvents.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace we::runtime::kindui {

class Widget;
class OverlayHost;

class KINDUI_API EventSystem {
public:
    EventSystem() = default;
    ~EventSystem();

    void SetRootWidget(const std::shared_ptr<Widget>& root) { m_Root = root; }
    std::shared_ptr<Widget> GetRootWidget() const { return m_Root; }

    void ProcessMouseEvent(const MouseEvent& event);
    void ProcessKeyEvent(const KeyEvent& event);
    void ProcessTextInput(char32_t codepoint);

    std::shared_ptr<Widget> GetFocusedWidget() const { return m_FocusedWidget.lock(); }
    std::shared_ptr<Widget> GetHoveredWidget() const { return m_HoveredWidget.lock(); }
    std::shared_ptr<Widget> GetCapturedWidget() const { return m_CapturedWidget.lock(); }

    void SetFocusedWidget(const std::shared_ptr<Widget>& widget);
    void SetCapturedWidget(const std::shared_ptr<Widget>& widget) { m_CapturedWidget = widget; }
    void SetSuppressSystemCursor(bool suppress) { m_SuppressSystemCursor = suppress; }

    /// Un-highlights the hovered widget and restores the system cursor. Call when
    /// the pointer leaves the window or the window loses focus so hover can never
    /// stay pinned on the last hit widget.
    void ClearHover();
    void ClearCapture() { m_CapturedWidget.reset(); }
    void ClearFocus() { SetFocusedWidget(nullptr); }
    void ClearAllInputState() { m_CapturedWidget.reset(); SetFocusedWidget(nullptr); ClearHover(); }

    void SetPopupHost(OverlayHost* popupHost) { m_PopupHost = popupHost; }

    /// Move keyboard focus to the next/previous focusable widget in tree order.
    void FocusNext(bool reverse = false);

    static std::shared_ptr<Widget> HitTest(const std::shared_ptr<Widget>& root, const Point& pos);

private:
    void UpdateCursorForWidget(const std::shared_ptr<Widget>& widget, const Point& position);
    void CollectFocusable(const std::shared_ptr<Widget>& node, std::vector<std::shared_ptr<Widget>>& out) const;

#pragma warning(push)
#pragma warning(disable: 4251)
    std::shared_ptr<Widget> m_Root;
    std::weak_ptr<Widget> m_FocusedWidget;
    std::weak_ptr<Widget> m_HoveredWidget;
    std::vector<std::weak_ptr<Widget>> m_HoverChain;
    std::weak_ptr<Widget> m_CapturedWidget;
#pragma warning(pop)
    OverlayHost* m_PopupHost = nullptr;
    Point m_LastMousePos{ -1.0f, -1.0f };
    bool m_UsingPointerCursor = false;
    bool m_SuppressSystemCursor = false;
};

} // namespace we::runtime::kindui
