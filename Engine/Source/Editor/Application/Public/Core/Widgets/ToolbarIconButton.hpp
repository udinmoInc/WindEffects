#pragma once

#include "Core/Widget.hpp"
#include "Core/ToolbarDesignTokens.hpp"
#include <functional>
#include <memory>
#include <string>

namespace we::UI {

// Circular icon button for toolbar navigation
// Used for Back, Forward, Folder, Refresh, etc.
class ToolbarIconButton : public Widget {
public:
    ToolbarIconButton(const std::string& iconName, const char* tooltip = nullptr);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    void SetSelected(bool selected) { m_Selected = selected; }
    bool IsEnabled() const { return m_Enabled; }
    bool IsSelected() const { return m_Selected; }

private:
    std::string m_IconName;
    bool m_Enabled = true;
    bool m_Selected = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_PressOffset = 0.0f;
    std::function<void()> m_OnClicked;
};

} // namespace we::UI
