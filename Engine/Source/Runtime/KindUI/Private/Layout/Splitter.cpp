#include "KindUI/Layout/Splitter.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include <algorithm>
#include <cmath>


namespace we::runtime::kindui {

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

void Splitter::SetSplitRatio(float ratio) {
    m_SplitRatio = std::clamp(ratio, 0.05f, 0.95f);
}

void Splitter::SetFixedFirstWidth(float width) {
    m_FixedFirstWidth = std::max(100.0f, width);
}

void Splitter::SetResizeMode(ResizeMode mode) {
    m_ResizeMode = mode;
}

float Splitter::GetEffectiveBarThickness() const {
    if (m_PanelGapEnabled) {
        return ResolveMetric(MetricToken::Space2);
    }
    return m_BarThickness;
}

Rect Splitter::GetSplitterBarRect() const {
    const float barThickness = GetEffectiveBarThickness();
    if (m_Orientation == Orientation::Horizontal) {
        float x;
        if (m_ResizeMode == ResizeMode::FixedFirst) {
            x = m_Geometry.x + m_FixedFirstWidth;
        } else {
            x = m_Geometry.x + (m_Geometry.width - barThickness) * m_SplitRatio;
        }
        return Rect{ x, m_Geometry.y, barThickness, m_Geometry.height };
    } else {
        float y = m_Geometry.y + (m_Geometry.height - barThickness) * m_SplitRatio;
        return Rect{ m_Geometry.x, y, m_Geometry.width, barThickness };
    }
}

Size Splitter::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize; // Fill all space

    const float barThickness = GetEffectiveBarThickness();
    float availW = availableSize.width;
    float availH = availableSize.height;

    if (m_Orientation == Orientation::Horizontal) {
        float w1, w2;
        if (m_ResizeMode == ResizeMode::FixedFirst && (m_FirstChild && m_FirstChild->IsVisible())) {
            w1 = m_FixedFirstWidth;
            w2 = availW - barThickness - w1;
            if (w2 < 1.0f) {
                w1 = availW - barThickness;
                w2 = 0.0f;
            }
        } else {
            const float firstRatio = (m_FirstChild && m_FirstChild->IsVisible()) ? m_SplitRatio : 0.0f;
            w1 = (availW - barThickness) * firstRatio;
            w2 = availW - barThickness - w1;
            if (w1 < 1.0f) {
                w1 = 0.0f;
                w2 = availW;
            }
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Measure(Size{ w1, availH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Measure(Size{ w2, availH });
        }
    } else {
        const float firstRatio = (m_FirstChild && m_FirstChild->IsVisible()) ? m_SplitRatio : 0.0f;
        float h1 = (availH - barThickness) * firstRatio;
        float h2 = availH - barThickness - h1;
        if (h1 < 1.0f) {
            h1 = 0.0f;
            h2 = availH;
        }

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

    const float barThickness = GetEffectiveBarThickness();
    float availW = allottedRect.width;
    float availH = allottedRect.height;

    if (m_Orientation == Orientation::Horizontal) {
        float w1, w2, barX;
        if (m_ResizeMode == ResizeMode::FixedFirst && (m_FirstChild && m_FirstChild->IsVisible())) {
            w1 = m_FixedFirstWidth;
            w2 = availW - barThickness - w1;
            barX = allottedRect.x + w1;
            if (w2 < 1.0f) {
                w1 = availW - barThickness;
                w2 = 0.0f;
                barX = allottedRect.x + w1;
            }
        } else {
            const float firstRatio = (m_FirstChild && m_FirstChild->IsVisible()) ? m_SplitRatio : 0.0f;
            w1 = (availW - barThickness) * firstRatio;
            w2 = availW - barThickness - w1;
            barX = allottedRect.x + w1;
            if (w1 < 1.0f) {
                w1 = 0.0f;
                w2 = availW;
                barX = allottedRect.x;
            }
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            float snappedX = std::round(allottedRect.x);
            float snappedY = std::round(allottedRect.y);
            float snappedW = std::round(allottedRect.x + w1) - snappedX;
            float snappedH = std::round(allottedRect.y + availH) - snappedY;
            m_FirstChild->Arrange(Rect{ snappedX, snappedY, snappedW, snappedH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            const float secondX = w1 > 0.0f ? barX + barThickness : allottedRect.x;
            float snappedX = std::round(secondX);
            float snappedY = std::round(allottedRect.y);
            float snappedW = std::round(secondX + w2) - snappedX;
            float snappedH = std::round(allottedRect.y + availH) - snappedY;
            m_SecondChild->Arrange(Rect{ snappedX, snappedY, snappedW, snappedH });
        }
    } else {
        const float firstRatio = (m_FirstChild && m_FirstChild->IsVisible()) ? m_SplitRatio : 0.0f;
        float h1 = (availH - barThickness) * firstRatio;
        float h2 = availH - barThickness - h1;
        float barY = allottedRect.y + h1;
        if (h1 < 1.0f) {
            h1 = 0.0f;
            h2 = availH;
            barY = allottedRect.y;
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            float snappedX = std::round(allottedRect.x);
            float snappedY = std::round(allottedRect.y);
            float snappedW = std::round(allottedRect.x + availW) - snappedX;
            float snappedH = std::round(allottedRect.y + h1) - snappedY;
            m_FirstChild->Arrange(Rect{ snappedX, snappedY, snappedW, snappedH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            const float secondY = h1 > 0.0f ? barY + barThickness : allottedRect.y;
            float snappedX = std::round(allottedRect.x);
            float snappedY = std::round(secondY);
            float snappedW = std::round(allottedRect.x + availW) - snappedX;
            float snappedH = std::round(secondY + h2) - snappedY;
            m_SecondChild->Arrange(Rect{ snappedX, snappedY, snappedW, snappedH });
        }
    }
}

Rect Splitter::GetSplitterHitRect() const {
    Rect barRect = GetSplitterBarRect();
    const float barThickness = GetEffectiveBarThickness();
    // Inflate the rect for grabbing
    if (m_Orientation == Orientation::Horizontal) {
        float padding = (m_HitThickness - barThickness) / 2.0f;
        return Rect{ barRect.x - padding, barRect.y, m_HitThickness, barRect.height };
    } else {
        float padding = (m_HitThickness - barThickness) / 2.0f;
        return Rect{ barRect.x, barRect.y - padding, barRect.width, m_HitThickness };
    }
}

void Splitter::Paint(PaintContext& context) {
    if (!m_Visible) return;

    if (m_PanelGapEnabled && m_FirstChild && m_FirstChild->IsVisible()) {
        Rect barRect = GetSplitterBarRect();
        context.DrawRect(barRect, ResolveColor(ColorToken::WorkspaceBackground));
    }

    if (m_FirstChild && m_FirstChild->IsVisible()) m_FirstChild->Paint(context);
    if (m_SecondChild && m_SecondChild->IsVisible()) m_SecondChild->Paint(context);

    if (!m_FirstChild || !m_FirstChild->IsVisible() || m_PanelGapEnabled) {
        return;
    }

    // Draw Splitter Bar
    Rect barRect = GetSplitterBarRect();
    
    // Draw an extremely subtle 1px line to separate panels cleanly
    Color subtleBorder = ThemeColor(ColorToken::BorderDefault); // Use theme border color
    
    if (m_Dragging || m_Hovered) {
        // Draw the soft blue highlight when hovered or dragging
        subtleBorder = ThemeColor(ColorToken::AccentPrimary);
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
            if (m_ResizeMode == ResizeMode::FixedFirst) {
                float relativeX = event.position.x - m_Geometry.x;
                m_FixedFirstWidth = std::clamp(relativeX, 100.0f, m_Geometry.width - 100.0f);
            } else {
                float relativeX = event.position.x - m_Geometry.x;
                m_SplitRatio = std::clamp(relativeX / m_Geometry.width, 0.05f, 0.95f);
            }
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

} // namespace we::runtime::kindui
