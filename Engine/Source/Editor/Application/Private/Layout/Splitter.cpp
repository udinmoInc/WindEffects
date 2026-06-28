#include "Layout/Splitter.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include <algorithm>

namespace we::UI {

Splitter::Splitter(Orientation orientation, float initialRatio)
    : m_Orientation(orientation), m_SplitRatio(initialRatio) {}

void Splitter::SetFirstChild(const std::shared_ptr<Widget>& child) {
    if (m_FirstChild) RemoveChild(m_FirstChild);
    m_FirstChild = child;
    AddChild(child);
}

void Splitter::SetSecondChild(const std::shared_ptr<Widget>& child) {
    if (m_SecondChild) RemoveChild(m_SecondChild);
    m_SecondChild = child;
    AddChild(child);
}

Rect Splitter::GetSplitterBarRect() const {
    if (m_Orientation == Orientation::Horizontal) {
        float x = m_Geometry.x + (m_Geometry.width - m_BarThickness) * m_SplitRatio;
        return Rect{ x, m_Geometry.y, m_BarThickness, m_Geometry.height };
    } else {
        float y = m_Geometry.y + (m_Geometry.height - m_BarThickness) * m_SplitRatio;
        return Rect{ m_Geometry.x, y, m_Geometry.width, m_BarThickness };
    }
}

Size Splitter::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize; // Fill all space

    float availW = availableSize.width;
    float availH = availableSize.height;

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = (availW - m_BarThickness) * m_SplitRatio;
        float w2 = (availW - m_BarThickness) * (1.0f - m_SplitRatio);

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Measure(Size{ w1, availH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Measure(Size{ w2, availH });
        }
    } else {
        float h1 = (availH - m_BarThickness) * m_SplitRatio;
        float h2 = (availH - m_BarThickness) * (1.0f - m_SplitRatio);

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Measure(Size{ availW, h1 });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Measure(Size{ availW, h2 });
        }
    }

    return m_DesiredSize;
}

void Splitter::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    float availW = allottedRect.width;
    float availH = allottedRect.height;

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = (availW - m_BarThickness) * m_SplitRatio;
        float w2 = (availW - m_BarThickness) * (1.0f - m_SplitRatio);
        float barX = allottedRect.x + w1;

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Arrange(Rect{ allottedRect.x, allottedRect.y, w1, availH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Arrange(Rect{ barX + m_BarThickness, allottedRect.y, w2, availH });
        }
    } else {
        float h1 = (availH - m_BarThickness) * m_SplitRatio;
        float h2 = (availH - m_BarThickness) * (1.0f - m_SplitRatio);
        float barY = allottedRect.y + h1;

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Arrange(Rect{ allottedRect.x, allottedRect.y, availW, h1 });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Arrange(Rect{ allottedRect.x, barY + m_BarThickness, availW, h2 });
        }
    }
}

Rect Splitter::GetSplitterHitRect() const {
    Rect barRect = GetSplitterBarRect();
    // Inflate the rect for grabbing
    if (m_Orientation == Orientation::Horizontal) {
        float padding = (m_HitThickness - m_BarThickness) / 2.0f;
        return Rect{ barRect.x - padding, barRect.y, m_HitThickness, barRect.height };
    } else {
        float padding = (m_HitThickness - m_BarThickness) / 2.0f;
        return Rect{ barRect.x, barRect.y - padding, barRect.width, m_HitThickness };
    }
}

void Splitter::Paint(PaintContext& context) {
    if (!m_Visible) return;

    if (m_FirstChild && m_FirstChild->IsVisible()) m_FirstChild->Paint(context);
    if (m_SecondChild && m_SecondChild->IsVisible()) m_SecondChild->Paint(context);

    // Draw Splitter Bar
    Rect barRect = GetSplitterBarRect();
    
    // Draw an extremely subtle 1px line to separate panels cleanly
    Color subtleBorder = Theme::Get().BorderDefault; // Use theme border color
    
    if (m_Dragging || m_Hovered) {
        // Draw the soft blue highlight when hovered or dragging
        subtleBorder = Theme::Get().SelectedAccent;
        if (!m_Dragging) {
            subtleBorder.a = 0.5f; 
        }
    }
    
    if (m_Orientation == Orientation::Horizontal) {
        Rect visualRect{ barRect.x + barRect.width / 2.0f, barRect.y, 1.0f, barRect.height };
        context.DrawRect(visualRect, subtleBorder);
    } else {
        Rect visualRect{ barRect.x, barRect.y + barRect.height / 2.0f, barRect.width, 1.0f };
        context.DrawRect(visualRect, subtleBorder);
    }
}

void Splitter::OnMouseDown(const MouseEvent& event) {
    Rect hitRect = GetSplitterHitRect();
    if (hitRect.Contains(event.position)) {
        m_Dragging = true;
    }
}

void Splitter::OnMouseMove(const MouseEvent& event) {
    Rect hitRect = GetSplitterHitRect();
    m_Hovered = hitRect.Contains(event.position);

    if (m_Dragging) {
        if (m_Orientation == Orientation::Horizontal) {
            float relativeX = event.position.x - m_Geometry.x;
            m_SplitRatio = std::clamp(relativeX / m_Geometry.width, 0.05f, 0.95f);
        } else {
            float relativeY = event.position.y - m_Geometry.y;
            m_SplitRatio = std::clamp(relativeY / m_Geometry.height, 0.05f, 0.95f);
        }
    }
}

void Splitter::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_Dragging = false;
}

} // namespace we::editor::application::UI
