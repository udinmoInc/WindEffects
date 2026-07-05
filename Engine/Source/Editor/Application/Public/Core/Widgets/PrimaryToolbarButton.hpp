#pragma once

#include "Application/Export.hpp"

#include "Core/Widget.hpp"
#include "Core/ToolbarDesignTokens.hpp"
#include <functional>
#include <memory>
#include <string>

namespace we::UI {

// Premium primary toolbar button with subtle 3D depth
// Used for primary actions like "Create"
class APPLICATION_API PrimaryToolbarButton : public Widget {
public:
    PrimaryToolbarButton(const std::string& label, const std::string& iconName = {});

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    std::string m_Label;
    std::string m_IconName;
    bool m_Enabled = true;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;
    float m_PressOffset = 0.0f;
    std::function<void()> m_OnClicked;
};

} // namespace we::UI
