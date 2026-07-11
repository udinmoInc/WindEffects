#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Widget.h"
#include <functional>
#include <memory>
#include <string>

namespace WindEffects::Editor::UI {

// Premium secondary toolbar button with flat appearance
// Used for secondary actions like "Import"
class UIFRAMEWORK_API SecondaryToolbarButton : public Widget {
public:
    SecondaryToolbarButton(const std::string& label, const std::string& iconName = {});

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

} // namespace WindEffects::Editor::UI
