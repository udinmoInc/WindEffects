#pragma once

#include "Application/Export.hpp"

#include "Core/Widget.hpp"

namespace we::UI {

class APPLICATION_API ScrollLayout : public Widget {
public:
    ScrollLayout();
    virtual ~ScrollLayout() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseWheel(const MouseEvent& event) override;

    void SetContent(const std::shared_ptr<Widget>& child);
    std::shared_ptr<Widget> GetContent() const { return m_Content; }

private:
    std::shared_ptr<Widget> m_Content;
    float m_ScrollOffset = 0.0f;
    float m_ScrollBarWidth = 4.0f;
};

} // namespace we::editor::application::UI
