#include "Layout/ScrollLayout.h"

#include "Core/PaintContext.h"
#include "Core/DPIContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>

namespace WindEffects::Editor::UI {

ScrollLayout::ScrollLayout() {}

void ScrollLayout::SetContent(const std::shared_ptr<Widget>& child) {
    if (m_Content) {
        RemoveChild(m_Content);
    }
    m_Content = child;
    if (m_Content) {
        AddChild(m_Content);
    }
}

float ScrollLayout::ContentHeight() const {
    return m_Content ? m_Content->GetDesiredSize().height : 0.0f;
}

float ScrollLayout::WheelStep() const {
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    return ResolveThemeMetric(ThemeToken::ListRowHeight) * uiScale;
}

void ScrollLayout::SyncScrollMetrics() {
    m_ContentHeight = ContentHeight();
    m_MaxScroll = ScrollViewport::MaxScroll(ViewportHeight(), m_ContentHeight);
    m_Scroll.Sync(ViewportHeight(), m_ContentHeight);
    m_Metrics = m_Scroll.ComputeMetrics(m_Geometry, m_ContentHeight, std::max(1.0f, DPIContext::GetScale()));
}

Size ScrollLayout::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;

    if (m_Content && m_Content->IsVisible()) {
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        const float contentWidth = std::max(
            0.0f,
            availableSize.width - ScrollViewport::ScrollbarWidth(uiScale));
        m_Content->Measure(Size{ contentWidth, 100000.0f });
    }

    return m_DesiredSize;
}

void ScrollLayout::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    SyncScrollMetrics();

    if (m_Content && m_Content->IsVisible()) {
        m_Content->Arrange(Rect{
            m_Metrics.viewport.x,
            m_Metrics.viewport.y - m_Scroll.offset,
            m_Metrics.viewport.width,
            m_ContentHeight
        });
    }
}

void ScrollLayout::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    SyncScrollMetrics();

    context.PushClipRect(m_Metrics.viewport);
    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(context);
    }
    context.PopClipRect();

    m_Scroll.Paint(context, m_Metrics, m_Scroll.IsThumbHovered());
}

void ScrollLayout::OnMouseDown(const MouseEvent& event) {
    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_Metrics, ViewportHeight(), m_ContentHeight)) {
        Arrange(m_Geometry);
        return;
    }

    if (m_Content && m_Metrics.viewport.Contains(event.position)) {
        m_Content->OnMouseDown(event);
    }
}

void ScrollLayout::OnMouseMove(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_Metrics, ViewportHeight(), m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        Arrange(m_Geometry);
        return;
    }

    if (m_Content && m_Metrics.viewport.Contains(event.position)) {
        m_Content->OnMouseMove(event);
    }
}

void ScrollLayout::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);
    if (m_Content && m_Metrics.viewport.Contains(event.position)) {
        m_Content->OnMouseUp(event);
    }
}

void ScrollLayout::OnMouseWheel(const MouseEvent& event) {
    if (!m_Metrics.viewport.Contains(event.position) && !m_Geometry.Contains(event.position)) {
        return;
    }

    SyncScrollMetrics();
    m_Scroll.ApplyWheel(event.wheelDeltaY, WheelStep(), ViewportHeight(), m_ContentHeight);
    Arrange(m_Geometry);
}

bool ScrollLayout::ShowsPointerCursor(const Point& position) const {
    return m_Metrics.showsScrollbar &&
        (m_Metrics.thumb.Contains(position) || m_Metrics.track.Contains(position));
}

void ScrollLayout::SetScrollOffset(float offset) {
    m_Scroll.SetOffset(offset, ViewportHeight(), m_ContentHeight);
    Arrange(m_Geometry);
}

bool ScrollLayout::ScrollToOffset(float offset) {
    const float previous = m_Scroll.offset;
    m_Scroll.SetOffset(offset, ViewportHeight(), m_ContentHeight);
    Arrange(m_Geometry);
    return std::abs(m_Scroll.offset - previous) > 0.01f;
}

bool ScrollLayout::ScrollToMakeVisible(const Rect& contentRect) {
    const bool changed = m_Scroll.ScrollToRange(
        contentRect.y,
        contentRect.y + contentRect.height,
        ViewportHeight(),
        m_ContentHeight);
    if (changed) {
        Arrange(m_Geometry);
    }
    return changed;
}

} // namespace WindEffects::Editor::UI
