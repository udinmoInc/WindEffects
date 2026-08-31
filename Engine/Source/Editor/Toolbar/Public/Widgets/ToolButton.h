#pragma once

#include "Toolbar/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/WindIcon.h"
#include <functional>

namespace we::editor::toolbar {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


enum class ToolButtonStyle {
    Normal,
    PlayButton,      // Legacy – kept for compatibility
    TransportButton, // Play / Pause / Stop – compact icon-only
    WindowControl,
    WindowClose,
    TitleBarTool,
    ToolbarIconOnly,
    ToolbarInline,    // Icon + optional label + chevron
    ToolbarLabeled,   // Icon above text label (transform tools)
    ViewportChip,     // Individual floating viewport control pill
    StatusBar         // Flat footer/status strip control
};

// Icon and text button for toolbar use
class TOOLBAR_API ToolButton : public Widget {
public:
    ToolButton(we::runtime::kindui::WindIconRef icon, const std::string& label = "", std::function<void()> onClicked = nullptr, const std::string& tooltip = "");
    virtual ~ToolButton() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    we::runtime::kindui::WindIconRef GetIcon() const { return m_Icon; }
    const std::string& GetTooltip() const { return m_Tooltip; }
    void SetIcon(we::runtime::kindui::WindIconRef icon) { m_Icon = icon; }
    void SetLabel(const std::string& label) { m_Label = label; }
    const std::string& GetLabel() const { return m_Label; }
    void SetOnMouseWheel(std::function<void(float wheelDeltaY)> onMouseWheel) { m_OnMouseWheel = std::move(onMouseWheel); }
    void SetTooltip(const std::string& tooltip) { m_Tooltip = tooltip; }
    void SetOnClicked(std::function<void()> onClicked) { m_OnClicked = onClicked; }
    void SetActive(bool active) { m_Active = active; }
    bool IsActive() const { return m_Active; }
    void SetButtonStyle(ToolButtonStyle style) { m_ButtonStyle = style; }
    void SetIsDropdown(bool isDropdown) { m_IsDropdown = isDropdown; }
    bool IsDropdown() const { return m_IsDropdown; }
    void SetChromeless(bool chromeless) { m_Chromeless = chromeless; }
    bool IsChromeless() const { return m_Chromeless; }

private:
    we::runtime::kindui::WindIconRef m_Icon = we::runtime::kindui::kWindIconNone;
    std::string m_Label;
    std::string m_Tooltip;
    std::function<void()> m_OnClicked;
    std::function<void(float)> m_OnMouseWheel;
    bool m_Active = false;
    bool m_Pressed = false;
    bool m_IsDropdown = false;
    bool m_Chromeless = false;
    ToolButtonStyle m_ButtonStyle = ToolButtonStyle::Normal;

    // Animation states [0.0, 1.0]
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_ActiveAnim = 0.0f;

    WidgetStyle m_Style;
};

// Separator for toolbar grouping
class ToolSeparator : public Widget {
public:
    ToolSeparator();
    virtual ~ToolSeparator() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    static float SeparatorHeight();
    static float SeparatorWidth();
};

} // namespace we::editor::toolbar