#include "KindUI/Layout/ScrollViewport.h"

#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cmath>


namespace we::runtime::kindui {
namespace {

constexpr float kMinThumbHeight = 24.0f;

float ThumbHeight(float trackHeight, float viewportHeight, float contentHeight) {
    if (contentHeight <= viewportHeight || trackHeight <= 0.0f) {
        return trackHeight;
    }
    return std::max(kMinThumbHeight, trackHeight * (viewportHeight / contentHeight));
}

} // namespace

float ScrollViewport::ScrollbarWidth(float uiScale) {
    return ResolveMetric(MetricToken::ScrollbarWidth) * uiScale;
}

float ScrollViewport::MaxScroll(float viewportHeight, float contentHeight) {
    return std::max(0.0f, contentHeight - viewportHeight);
}

float ScrollViewport::ClampOffset(float value, float viewportHeight, float contentHeight) {
    return std::clamp(value, 0.0f, MaxScroll(viewportHeight, contentHeight));
}

bool ScrollViewport::NeedsScrollbar(float viewportHeight, float contentHeight) {
    return contentHeight > viewportHeight + 0.5f;
}

void ScrollViewport::Sync(float viewportHeight, float contentHeight) {
    offset = ClampOffset(offset, viewportHeight, contentHeight);
}

void ScrollViewport::ApplyWheel(float deltaY, float pixelsPerTick, float viewportHeight, float contentHeight) {
    offset = ClampOffset(offset - deltaY * pixelsPerTick, viewportHeight, contentHeight);
}

void ScrollViewport::SetOffset(float value, float viewportHeight, float contentHeight) {
    offset = ClampOffset(value, viewportHeight, contentHeight);
}

bool ScrollViewport::ScrollToRange(float itemTop, float itemBottom, float viewportHeight, float contentHeight) {
    const float previous = offset;
    if (itemTop < offset) {
        offset = itemTop;
    } else if (itemBottom > offset + viewportHeight) {
        offset = itemBottom - viewportHeight;
    }
    offset = ClampOffset(offset, viewportHeight, contentHeight);
    return std::abs(offset - previous) > 0.01f;
}

ScrollViewportMetrics ScrollViewport::ComputeMetrics(
    const Rect& bounds,
    float contentHeight,
    float uiScale) const
{
    ScrollViewportMetrics metrics{};
    metrics.showsScrollbar = NeedsScrollbar(bounds.height, contentHeight);
    metrics.scrollbarWidth = metrics.showsScrollbar ? ScrollbarWidth(uiScale) : 0.0f;

    metrics.viewport = Rect{
        bounds.x,
        bounds.y,
        std::max(0.0f, bounds.width - metrics.scrollbarWidth),
        bounds.height
    };

    if (!metrics.showsScrollbar) {
        return metrics;
    }

    metrics.track = Rect{
        bounds.x + bounds.width - metrics.scrollbarWidth,
        bounds.y,
        metrics.scrollbarWidth,
        bounds.height
    };

    const float maxScroll = MaxScroll(bounds.height, contentHeight);
    const float thumbHeight = ThumbHeight(metrics.track.height, bounds.height, contentHeight);
    const float scrollRatio = maxScroll > 0.0f ? (offset / maxScroll) : 0.0f;
    const float thumbY = metrics.track.y + (metrics.track.height - thumbHeight) * scrollRatio;

    metrics.thumb = Rect{
        metrics.track.x + metrics.scrollbarWidth * 0.15f,
        thumbY,
        metrics.track.width * 0.7f,
        thumbHeight
    };

    return metrics;
}

void ScrollViewport::Paint(
    PaintContext& context,
    const ScrollViewportMetrics& metrics,
    bool thumbHovered) const
{
    if (!metrics.showsScrollbar) {
        return;
    }

    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Scrollbar);
    context.DrawRect(metrics.track, style.background);

    Color thumbColor = thumbHovered || m_DraggingThumb ? style.border : style.foreground;
    context.DrawRoundedRect(metrics.thumb, thumbColor, metrics.thumb.width * 0.5f);
}

bool ScrollViewport::OnMouseDown(
    const MouseEvent& event,
    const ScrollViewportMetrics& metrics,
    float viewportHeight,
    float contentHeight)
{
    if (event.button != MouseButton::Left || !metrics.showsScrollbar) {
        return false;
    }

    if (metrics.thumb.Contains(event.position)) {
        m_DraggingThumb = true;
        m_DragStartY = event.position.y;
        m_DragStartOffset = offset;
        return true;
    }

    if (metrics.track.Contains(event.position)) {
        JumpToTrack(event.position.y - metrics.track.y, metrics, viewportHeight, contentHeight);
        m_DraggingThumb = true;
        m_DragStartY = event.position.y;
        m_DragStartOffset = offset;
        return true;
    }

    return false;
}

void ScrollViewport::OnMouseMove(
    const MouseEvent& event,
    const ScrollViewportMetrics& metrics,
    float viewportHeight,
    float contentHeight)
{
    m_ThumbHovered = metrics.showsScrollbar && metrics.thumb.Contains(event.position);

    if (!m_DraggingThumb || !metrics.showsScrollbar) {
        return;
    }

    const float maxScroll = MaxScroll(viewportHeight, contentHeight);
    const float thumbHeight = ThumbHeight(metrics.track.height, viewportHeight, contentHeight);
    const float trackTravel = std::max(1.0f, metrics.track.height - thumbHeight);
    const float deltaY = event.position.y - m_DragStartY;
    offset = ClampOffset(m_DragStartOffset + (deltaY / trackTravel) * maxScroll, viewportHeight, contentHeight);
}

void ScrollViewport::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_DraggingThumb = false;
}

void ScrollViewport::JumpToTrack(
    float localY,
    const ScrollViewportMetrics& metrics,
    float viewportHeight,
    float contentHeight)
{
    const float maxScroll = MaxScroll(viewportHeight, contentHeight);
    const float thumbHeight = ThumbHeight(metrics.track.height, viewportHeight, contentHeight);
    const float trackTravel = std::max(1.0f, metrics.track.height - thumbHeight);
    const float centeredY = std::clamp(localY - thumbHeight * 0.5f, 0.0f, trackTravel);
    offset = ClampOffset((centeredY / trackTravel) * maxScroll, viewportHeight, contentHeight);
}

} // namespace we::runtime::kindui
