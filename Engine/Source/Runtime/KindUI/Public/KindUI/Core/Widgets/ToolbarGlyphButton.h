#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/Widget.h"

#include <functional>
#include <string>
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

/// Shared toolbar glyph button (icon-only). Used by ToolbarIconButton and ToolbarNavigationButton.
class KINDUI_API ToolbarGlyphButton : public Widget {
public:
    ToolbarGlyphButton(std::string iconName, StyleRole role, MetricToken sizeToken, MetricToken iconSizeToken);

    void SetOnClicked(std::function<void()> callback) { m_OnClicked = std::move(callback); }
    void SetSelected(bool selected) { Widget::SetSelected(selected); }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void Tick(float deltaTime) override;
    bool ShowsPointerCursor(const Point& position) const override { return IsEnabled() && m_Geometry.Contains(position); }

private:
    std::string m_IconName;
    StyleRole m_Role = StyleRole::IconButton;
    MetricToken m_SizeToken = MetricToken::IconButtonSize;
    MetricToken m_IconSizeToken = MetricToken::IconSizeToolbar;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_PressOffset = 0.0f;
    std::function<void()> m_OnClicked;
};

} // namespace we::runtime::kindui
