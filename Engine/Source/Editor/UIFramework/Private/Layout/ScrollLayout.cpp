#include "Layout/ScrollLayout.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

ScrollLayout::ScrollLayout() {}

void ScrollLayout::SetContent(const std::shared_ptr<Widget>& child) {
    if (m_Content) RemoveChild(m_Content);
    m_Content = child;
    AddChild(child);
}

Size ScrollLayout::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;

    if (m_Content && m_Content->IsVisible()) {
        // Measure with infinite height to find total scrolling size
        m_Content->Measure(Size{ availableSize.width - m_ScrollBarWidth, 100000.0f });
    }

    return m_DesiredSize;
}

void ScrollLayout::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    if (m_Content && m_Content->IsVisible()) {
        float contentHeight = m_Content->GetDesiredSize().height;
        float maxScroll = std::max(0.0f, contentHeight - allottedRect.height);
        m_ScrollOffset = std::clamp(m_ScrollOffset, 0.0f, maxScroll);

        // Arrange child with its full height offset by scroll offset
        m_Content->Arrange(Rect{
            allottedRect.x,
            allottedRect.y - m_ScrollOffset,
            allottedRect.width - m_ScrollBarWidth,
            contentHeight
        });
    }
}

void ScrollLayout::Paint(PaintContext& context) {
    if (!m_Visible) return;

    // 1. Push scissor clip rect
    context.PushClipRect(m_Geometry);

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }

    // 2. Pop scissor clip rect
    context.PopClipRect();

    // 3. Draw Scrollbar if content exceeds height
    if (m_Content && m_Content->IsVisible()) {
        float contentHeight = m_Content->GetDesiredSize().height;
        if (contentHeight > m_Geometry.height) {
            float trackX = m_Geometry.x + m_Geometry.width - m_ScrollBarWidth;
            float trackY = m_Geometry.y;
            float trackW = m_ScrollBarWidth;
            float trackH = m_Geometry.height;

            // Draw track (very dark)
            context.DrawRect(Rect{ trackX, trackY, trackW, trackH }, ResolveThemeColor(ThemeToken::ScrollbarTrack));

            // Draw thumb
            float ratio = m_Geometry.height / contentHeight;
            float thumbH = std::max(20.0f, trackH * ratio);
            float maxScroll = contentHeight - m_Geometry.height;
            float scrollRatio = (maxScroll > 0.0f) ? (m_ScrollOffset / maxScroll) : 0.0f;
            float thumbY = trackY + (trackH - thumbH) * scrollRatio;

            context.DrawRect(Rect{ trackX, thumbY, trackW, thumbH }, ResolveThemeColor(ThemeToken::ScrollbarThumb), 2.0f);
        }
    }
}

void ScrollLayout::OnMouseWheel(const MouseEvent& event) {
    if (m_Content && m_Content->IsVisible()) {
        float contentHeight = m_Content->GetDesiredSize().height;
        float maxScroll = std::max(0.0f, contentHeight - m_Geometry.height);

        // Adjust scroll offset (standard scroll speed)
        m_ScrollOffset -= event.wheelDeltaY * 20.0f;
        m_ScrollOffset = std::clamp(m_ScrollOffset, 0.0f, maxScroll);

        // Re-arrange content instantly
        Arrange(m_Geometry);
    }
}

} // namespace WindEffects::Editor::UI
