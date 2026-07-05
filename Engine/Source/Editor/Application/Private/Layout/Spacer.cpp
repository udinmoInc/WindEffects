#include "Layout/Spacer.hpp"

namespace we::UI {

Spacer::Spacer() = default;

Spacer::~Spacer() = default;

Size Spacer::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ 10000.0f, 10000.0f };
    return m_DesiredSize;
}

void Spacer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Spacer::Paint(PaintContext& context) {
    (void)context;
}

} // namespace we::UI
