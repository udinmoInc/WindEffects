#pragma once

#include "../Core/Widget.hpp"
#include "Box.hpp"

namespace HouseEngine::UI {

class Splitter : public Widget {
public:
    Splitter(Orientation orientation, float initialRatio = 0.5f);
    virtual ~Splitter() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;

    void SetFirstChild(const std::shared_ptr<Widget>& child);
    void SetSecondChild(const std::shared_ptr<Widget>& child);

private:
    Rect GetSplitterBarRect() const;
    Rect GetSplitterHitRect() const; // Wider area for grabbing

    Orientation m_Orientation;
    float m_SplitRatio = 0.5f;
    float m_BarThickness = 2.0f; // Visual layout thickness
    float m_HitThickness = 8.0f; // Transparent grab area
    bool m_Dragging = false;
    bool m_Hovered = false;

    std::shared_ptr<Widget> m_FirstChild;
    std::shared_ptr<Widget> m_SecondChild;
};

} // namespace HouseEngine::UI
