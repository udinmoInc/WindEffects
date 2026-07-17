#include "KindUI/Layout/Spacer.h"

namespace we::runtime::kindui {

Spacer::Spacer() = default;

Spacer::~Spacer() = default;

Size Spacer::Measure(const Size& availableSize) {
    (void)availableSize;
    // Desired size is zero on both axes. Flex grow/shrink (set by UI::Spacer)
    // absorbs free space along the main axis without inflating the cross axis
    // (a large desired width would force Column parents to become enormous).
    m_DesiredSize = Size{ 0.0f, 0.0f };
    return m_DesiredSize;
}

void Spacer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Spacer::Paint(PaintContext& context) {
    (void)context;
}

} // namespace we::runtime::kindui
