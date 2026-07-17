#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/ControlChrome.h"
#include "WindEffects/Runtime/UI/Core/Widget.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

/// Shared toolbar glyph button (icon-only). Used by ToolbarIconButton and ToolbarNavigationButton.
class UI_API ToolbarGlyphButton : public Widget {
public:
    ToolbarGlyphButton(std::string iconName, StyleRole role, ThemeToken sizeToken, ThemeToken iconSizeToken);

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
    ThemeToken m_SizeToken = ThemeToken::IconButtonSize;
    ThemeToken m_IconSizeToken = ThemeToken::IconSizeToolbar;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_PressOffset = 0.0f;
    std::function<void()> m_OnClicked;
};

} // namespace WindEffects::Editor::UI
