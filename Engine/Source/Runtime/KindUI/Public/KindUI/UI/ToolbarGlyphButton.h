// ==============================================================================
// WindEffects — KindUI — ToolbarGlyphButton
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"

#include <functional>
#include <string>
#include "KindUI/Theme/DesignToken.h"

namespace we::runtime::kindui {

/// Shared toolbar glyph button (icon-only). Used by ToolbarIconButton and ToolbarNavigationButton.
class KINDUI_API ToolbarGlyphButton : public Widget {
public:
    ToolbarGlyphButton(WindIconRef icon, StyleRole role, MetricToken sizeToken, MetricToken iconSizeToken);

    void SetOnClicked(std::function<void()> callback);
    void SetSelected(bool selected) { Widget::SetSelected(selected); }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return IsEnabled() &&
        m_Geometry.Contains(position); }

private:
    WindIconRef m_Icon = kWindIconNone;
    StyleRole m_Role = StyleRole::IconButton;
    MetricToken m_SizeToken = MetricToken::IconButtonSize;
    MetricToken m_IconSizeToken = MetricToken::IconSizeToolbar;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_PressOffset = 0.0f;
    std::function<void()> m_OnClicked;
};

class KINDUI_API ToolbarIconButton : public ToolbarGlyphButton {
public:
    explicit ToolbarIconButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
            StyleRole::IconButton,
            MetricToken::IconButtonSize,
            MetricToken::IconSizeToolbar)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback) { ToolbarGlyphButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { Widget::SetEnabled(enabled); }
    void SetSelected(bool selected) { ToolbarGlyphButton::SetSelected(selected); }
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

class KINDUI_API ToolbarNavigationButton : public ToolbarGlyphButton {
public:
    explicit ToolbarNavigationButton(WindIconRef icon, const char* tooltip = nullptr)
        : ToolbarGlyphButton(
            icon,
            StyleRole::NavigationButton,
            MetricToken::NavigationButtonSize,
            MetricToken::IconSizeWindowControl)
    {
        (void)tooltip;
    }

    void SetOnClicked(std::function<void()> callback) { ToolbarGlyphButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { Widget::SetEnabled(enabled); }
    void SetSelected(bool selected) { ToolbarGlyphButton::SetSelected(selected); }
    [[nodiscard]] bool IsEnabled() const { return Widget::IsEnabled(); }
    [[nodiscard]] bool IsSelected() const { return Widget::IsSelected(); }
};

} // namespace we::runtime::kindui
