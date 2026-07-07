#pragma once

#include "Toolbar/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include "Core/Icon.h"
#include "Core/Animation.h"
#include <functional>

namespace we::UI {

enum class ToolButtonStyle {
    Normal,
    PlayButton,      // Legacy – kept for compatibility
    TransportButton, // Play / Pause / Stop – compact icon-only
    WindowControl,
    WindowClose,
    TitleBarTool,
    ToolbarIconOnly,
    ToolbarInline    // Transparent inline item – icon + label + chevron, hover only
};

// Icon and text button for toolbar use
class TOOLBAR_API ToolButton : public Widget {
public:
    ToolButton(const std::string& iconName, const std::string& label = "", std::function<void()> onClicked = nullptr, const std::string& tooltip = "");
    virtual ~ToolButton() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetIcon(const std::string& iconName) { m_IconName = iconName; }
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

private:
    std::string m_IconName;
    std::string m_Label;
    std::string m_Tooltip;
    std::function<void()> m_OnClicked;
    std::function<void(float)> m_OnMouseWheel;
    bool m_Active = false;
    bool m_Pressed = false;
    bool m_IsDropdown = false;
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

    static constexpr float SEPARATOR_WIDTH = 1.0f;
    static constexpr float SEPARATOR_HEIGHT = 24.0f;
};

} // namespace we::editor::toolbar::UI
