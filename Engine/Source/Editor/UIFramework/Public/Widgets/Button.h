#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include <functional>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API Button : public Widget {
public:
    Button(const std::string& labelText, std::function<void()> onClicked = nullptr);
    virtual ~Button() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    void SetText(const std::string& text) { m_Text = text; }
    void SetOnClicked(std::function<void()> onClicked) { m_OnClicked = onClicked; }
    void SetStyle(const WidgetStyle& style) { m_Style = style; }

private:
    std::string m_Text;
    std::function<void()> m_OnClicked;
    bool m_Pressed = false;

    // Animation states [0.0, 1.0]
    float m_HoverAnim = 0.0f;
    float m_PressAnim = 0.0f;

    WidgetStyle m_Style;
};

} // namespace WindEffects::Editor::UI
