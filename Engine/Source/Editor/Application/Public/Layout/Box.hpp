#pragma once

#include "Core/Widget.hpp"

namespace we::UI {

enum class Orientation {
    Horizontal,
    Vertical
};

class Box : public Widget {
public:
    Box(Orientation orientation) : m_Orientation(orientation) {}
    virtual ~Box() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetSpacing(float spacing) { m_Spacing = spacing; }
    void SetPadding(const Margin& padding) { m_Padding = padding; }

private:
    Orientation m_Orientation;
    float m_Spacing = 2.0f;
    Margin m_Padding = { 0.0f, 0.0f, 0.0f, 0.0f };
};

class HorizontalBox : public Box {
public:
    HorizontalBox() : Box(Orientation::Horizontal) {}
};

class VerticalBox : public Box {
public:
    VerticalBox() : Box(Orientation::Vertical) {}
};

} // namespace we::editor::application::UI
