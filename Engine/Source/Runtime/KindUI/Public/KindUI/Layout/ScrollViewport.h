#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/EventSystem.h"

namespace we::runtime::kindui {

class PaintContext;

struct KINDUI_API ScrollViewportMetrics {
    Rect viewport{};
    Rect track{};
    Rect thumb{};
    float scrollbarWidth = 0.0f;
    bool showsScrollbar = false;
};

// Shared vertical scroll state used by ScrollLayout and immediate-mode list widgets.
class KINDUI_API ScrollViewport {
public:
    float offset = 0.0f;

    [[nodiscard]] static float ScrollbarWidth(float uiScale);
    [[nodiscard]] static float MaxScroll(float viewportHeight, float contentHeight);
    [[nodiscard]] static float ClampOffset(float offset, float viewportHeight, float contentHeight);
    [[nodiscard]] static bool NeedsScrollbar(float viewportHeight, float contentHeight);

    void Sync(float viewportHeight, float contentHeight);
    void ApplyWheel(float deltaY, float pixelsPerTick, float viewportHeight, float contentHeight);
    void SetOffset(float value, float viewportHeight, float contentHeight);
    bool ScrollToRange(float itemTop, float itemBottom, float viewportHeight, float contentHeight);

    [[nodiscard]] ScrollViewportMetrics ComputeMetrics(
        const Rect& bounds,
        float contentHeight,
        float uiScale) const;

    void Paint(PaintContext& context, const ScrollViewportMetrics& metrics, bool thumbHovered) const;

    bool OnMouseDown(const MouseEvent& event, const ScrollViewportMetrics& metrics, float viewportHeight, float contentHeight);
    void OnMouseMove(const MouseEvent& event, const ScrollViewportMetrics& metrics, float viewportHeight, float contentHeight);
    void OnMouseUp(const MouseEvent& event);

    [[nodiscard]] bool IsDraggingThumb() const { return m_DraggingThumb; }
    [[nodiscard]] bool IsThumbHovered() const { return m_ThumbHovered; }

private:
    void JumpToTrack(
        float localY,
        const ScrollViewportMetrics& metrics,
        float viewportHeight,
        float contentHeight);

    bool m_DraggingThumb = false;
    bool m_ThumbHovered = false;
    float m_DragStartY = 0.0f;
    float m_DragStartOffset = 0.0f;
};

} // namespace we::runtime::kindui
