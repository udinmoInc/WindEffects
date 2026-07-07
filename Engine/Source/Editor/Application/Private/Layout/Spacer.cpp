#include "Layout/Spacer.h"

namespace we::UI {

Spacer::Spacer() = default;

Spacer::~Spacer() = default;

Size Spacer::Measure(const Size& availableSize) {
    (void)availableSize;
    // Spacer should stretch along the box axis without forcing the cross-axis
    // to become enormous, otherwise parent boxes center their real children.
    m_DesiredSize = Size{ 10000.0f, 0.0f };
    return m_DesiredSize;
}

void Spacer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Spacer::Paint(PaintContext& context) {
    (void)context;
}

} // namespace we::UI
