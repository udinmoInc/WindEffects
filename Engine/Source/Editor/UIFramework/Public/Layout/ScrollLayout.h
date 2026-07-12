#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Widget.h"
#include "Layout/ScrollViewport.h"

namespace WindEffects::Editor::UI {

// Generic vertical scroll container with clipping, wheel/track/thumb interaction,
// and a themed scrollbar that appears only when content overflows.
class UIFRAMEWORK_API ScrollLayout : public Widget {
public:
    ScrollLayout();
    ~ScrollLayout() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void SetContent(const std::shared_ptr<Widget>& child);
    [[nodiscard]] std::shared_ptr<Widget> GetContent() const { return m_Content; }

    void SetScrollOffset(float offset);
    [[nodiscard]] float GetScrollOffset() const { return m_Scroll.offset; }
    [[nodiscard]] float GetMaxScrollOffset() const { return m_MaxScroll; }
    bool ScrollToOffset(float offset);
    bool ScrollToMakeVisible(const Rect& contentRect);

private:
    void SyncScrollMetrics();
    [[nodiscard]] float ViewportHeight() const { return m_Geometry.height; }
    [[nodiscard]] float ContentHeight() const;
    [[nodiscard]] float WheelStep() const;

    std::shared_ptr<Widget> m_Content;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_Metrics{};
    float m_ContentHeight = 0.0f;
    float m_MaxScroll = 0.0f;
};

} // namespace WindEffects::Editor::UI
