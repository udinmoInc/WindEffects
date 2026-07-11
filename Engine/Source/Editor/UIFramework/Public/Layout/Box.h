#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Widget.h"

namespace WindEffects::Editor::UI {

enum class Orientation {
    Horizontal,
    Vertical
};

class UIFRAMEWORK_API Box : public Widget {
public:
    Box(Orientation orientation) : m_Orientation(orientation) {}
    virtual ~Box() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetSpacing(float spacing) { m_Spacing = spacing; }
    void SetPadding(const Margin& padding) { m_Padding = padding; }
    void SetBackgroundColor(const Color& color) { m_BackgroundColor = color; m_HasBackground = true; }

private:
    Orientation m_Orientation;
    float m_Spacing = 2.0f;
    Margin m_Padding = { 0.0f, 0.0f, 0.0f, 0.0f };
    Color m_BackgroundColor{};
    bool m_HasBackground = false;
};

class UIFRAMEWORK_API HorizontalBox : public Box {
public:
    HorizontalBox() : Box(Orientation::Horizontal) {}
};

class UIFRAMEWORK_API VerticalBox : public Box {
public:
    VerticalBox() : Box(Orientation::Vertical) {}
};

} // namespace WindEffects::Editor::UI
