#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
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
    m_SplitRatio = std::clamp(ratio, 0.0f, 1.0f);
}

void Splitter::SetFixedFirstWidth(float width) {
    m_FixedFirstWidth = std::max(1.0f, width);
}

void Splitter::SetResizeMode(ResizeMode mode) {
    m_ResizeMode = mode;
}

void Splitter::SetMinPaneSizes(float minFirstPx, float minSecondPx) {
    m_MinFirstPx = std::max(0.0f, minFirstPx);
    m_MinSecondPx = std::max(0.0f, minSecondPx);
}

float Splitter::GetEffectiveBarThickness() const {
    const float scale = DPIContext::GetScale();
    if (m_PanelGapEnabled) {
        return DPIContext::Snap(ResolveMetric(MetricToken::Space2) * scale);
    }
    return DPIContext::Snap(m_BarThicknessLogical * scale);
}

void Splitter::ClampSplitToMins(float availMain, float barThickness) {
    const float usable = std::max(0.0f, availMain - barThickness);
    if (usable <= 0.0f) {
        m_SplitRatio = 0.0f;
        return;
    }

    float minFirst = m_MinFirstPx;
    float minSecond = m_MinSecondPx;
    if (minFirst + minSecond > usable) {
        const float scale = usable / (minFirst + minSecond);
        minFirst *= scale;
        minSecond *= scale;
    }

    const float minRatio = minFirst / usable;
    const float maxRatio = 1.0f - (minSecond / usable);
    m_SplitRatio = std::clamp(m_SplitRatio, minRatio, std::max(minRatio, maxRatio));
}

void Splitter::SplitAvailable(float availMain, float barThickness, float& first, float& second) const {
    const float usable = std::max(0.0f, availMain - barThickness);
    first = usable * m_SplitRatio;
    second = usable - first;
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
    }

    const float y = m_Geometry.y + (m_Geometry.height - barThickness) * m_SplitRatio;
    return Rect{ m_Geometry.x, y, m_Geometry.width, barThickness };
}

Size Splitter::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    AssertNonNegativeSize(m_SlotId.empty() ? "Splitter" : m_SlotId, availableSize.width, availableSize.height);

    const float barThickness = GetEffectiveBarThickness();
    float availW = std::max(0.0f, availableSize.width);
    float availH = std::max(0.0f, availableSize.height);

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = 0.0f;
        float w2 = 0.0f;
        if (m_ResizeMode == ResizeMode::FixedFirst && (m_FirstChild && m_FirstChild->IsVisible())) {
            w1 = std::min(m_FixedFirstWidth, std::max(0.0f, availW - barThickness));
            w2 = std::max(0.0f, availW - barThickness - w1);
        } else {
            const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
            if (!firstVisible) {
                w1 = 0.0f;
                w2 = availW;
            } else {
                ClampSplitToMins(availW, barThickness);
                SplitAvailable(availW, barThickness, w1, w2);
            }
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            m_FirstChild->Measure(Size{ w1, availH });
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            m_SecondChild->Measure(Size{ w2, availH });
        }
    } else {
        float h1 = 0.0f;
        float h2 = 0.0f;
        const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
        if (!firstVisible) {
            h1 = 0.0f;
            h2 = availH;
        } else {
            ClampSplitToMins(availH, barThickness);
            SplitAvailable(availH, barThickness, h1, h2);
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
    AssertNonNegativeSize(m_SlotId.empty() ? "Splitter" : m_SlotId, allottedRect.width, allottedRect.height);

    const float barThickness = GetEffectiveBarThickness();
    float availW = std::max(0.0f, allottedRect.width);
    float availH = std::max(0.0f, allottedRect.height);

    if (m_Orientation == Orientation::Horizontal) {
        float w1 = 0.0f;
        float w2 = 0.0f;
        float barX = allottedRect.x;
        if (m_ResizeMode == ResizeMode::FixedFirst && (m_FirstChild && m_FirstChild->IsVisible())) {
            w1 = std::min(m_FixedFirstWidth, std::max(0.0f, availW - barThickness));
            w2 = std::max(0.0f, availW - barThickness - w1);
            barX = allottedRect.x + w1;
        } else {
            const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
            if (!firstVisible) {
                w1 = 0.0f;
                w2 = availW;
                barX = allottedRect.x;
            } else {
                ClampSplitToMins(availW, barThickness);
                SplitAvailable(availW, barThickness, w1, w2);
                barX = allottedRect.x + w1;
            }
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            Rect firstRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(allottedRect.y),
                    std::round(allottedRect.x + w1) - std::round(allottedRect.x),
                    std::round(allottedRect.y + availH) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.first", firstRect, allottedRect);
            m_FirstChild->Arrange(firstRect);
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            const float secondX = w1 > 0.0f ? barX + barThickness : allottedRect.x;
            Rect secondRect = ClampRectToParent(
                Rect{
                    std::round(secondX),
                    std::round(allottedRect.y),
                    std::round(secondX + w2) - std::round(secondX),
                    std::round(allottedRect.y + availH) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.second", secondRect, allottedRect);
            m_SecondChild->Arrange(secondRect);
        }
    } else {
        float h1 = 0.0f;
        float h2 = 0.0f;
        float barY = allottedRect.y;
        const bool firstVisible = m_FirstChild && m_FirstChild->IsVisible();
        if (!firstVisible) {
            h1 = 0.0f;
            h2 = availH;
            barY = allottedRect.y;
        } else {
            ClampSplitToMins(availH, barThickness);
            SplitAvailable(availH, barThickness, h1, h2);
            barY = allottedRect.y + h1;
        }

        if (m_FirstChild && m_FirstChild->IsVisible()) {
            Rect firstRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(allottedRect.y),
                    std::round(allottedRect.x + availW) - std::round(allottedRect.x),
                    std::round(allottedRect.y + h1) - std::round(allottedRect.y)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.first", firstRect, allottedRect);
            m_FirstChild->Arrange(firstRect);
        }
        if (m_SecondChild && m_SecondChild->IsVisible()) {
            const float secondY = h1 > 0.0f ? barY + barThickness : allottedRect.y;
            Rect secondRect = ClampRectToParent(
                Rect{
                    std::round(allottedRect.x),
                    std::round(secondY),
                    std::round(allottedRect.x + availW) - std::round(allottedRect.x),
                    std::round(secondY + h2) - std::round(secondY)
                },
                allottedRect);
            AssertLayoutRectValid("Splitter.second", secondRect, allottedRect);
            m_SecondChild->Arrange(secondRect);
        }
    }
}

Rect Splitter::GetSplitterHitRect() const {
    Rect barRect = GetSplitterBarRect();
    const float barThickness = GetEffectiveBarThickness();
    const float hitThickness = DPIContext::Snap(m_HitThicknessLogical * DPIContext::GetScale());
    if (m_Orientation == Orientation::Horizontal) {
        const float padding = (hitThickness - barThickness) * 0.5f;
        return Rect{ barRect.x - padding, barRect.y, hitThickness, barRect.height };
    }
    const float padding = (hitThickness - barThickness) * 0.5f;
    return Rect{ barRect.x, barRect.y - padding, barRect.width, hitThickness };
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

    Rect barRect = GetSplitterBarRect();
    Color subtleBorder = ThemeColor(ColorToken::BorderDefault);
    if (m_Dragging || m_Hovered) {
        subtleBorder = ThemeColor(ColorToken::AccentPrimary);
        if (!m_Dragging) {
            subtleBorder.a = 0.5f;
        }
    }

    if (m_Orientation == Orientation::Horizontal) {
        Rect visualRect{ barRect.x + barRect.width * 0.5f, barRect.y, 1.0f, barRect.height };
        context.DrawRect(visualRect, subtleBorder);
    } else {
        Rect visualRect{ barRect.x, barRect.y + barRect.height * 0.5f, barRect.width, 1.0f };
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

    if (!m_Dragging) {
        return;
    }

    const float barThickness = GetEffectiveBarThickness();
    if (m_Orientation == Orientation::Horizontal) {
        if (m_ResizeMode == ResizeMode::FixedFirst) {
            const float relativeX = event.position.x - m_Geometry.x;
            const float maxFirst = std::max(m_MinFirstPx, m_Geometry.width - barThickness - m_MinSecondPx);
            m_FixedFirstWidth = std::clamp(relativeX, m_MinFirstPx, maxFirst);
        } else {
            const float usable = std::max(1.0f, m_Geometry.width - barThickness);
            const float relativeX = event.position.x - m_Geometry.x;
            m_SplitRatio = relativeX / usable;
            ClampSplitToMins(m_Geometry.width, barThickness);
        }
        InvalidateLayout();
    } else {
        const float usable = std::max(1.0f, m_Geometry.height - barThickness);
        const float relativeY = event.position.y - m_Geometry.y;
        m_SplitRatio = relativeY / usable;
        ClampSplitToMins(m_Geometry.height, barThickness);
        InvalidateLayout();
    }
}

void Splitter::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_Dragging = false;
}

} // namespace we::runtime::kindui
