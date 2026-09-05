#include "KindUI/Core/Widgets/VerticalDivider.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

VerticalDivider::VerticalDivider() = default;

Size VerticalDivider::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    float h = m_ExplicitHeight > 0.0f
        ? m_ExplicitHeight * uiScale
        : (availableSize.height > 0.0f ? availableSize.height * m_HeightRatio : 14.0f * uiScale);
    m_DesiredSize = Size{
        ds::Toolbar::SeparatorWidth() * uiScale,
        h
    };
    return m_DesiredSize;
}

void VerticalDivider::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    float h = allottedRect.height;
    if (m_HeightRatio > 0.0f && m_HeightRatio < 1.0f) {
        h = std::round(allottedRect.height * m_HeightRatio);
    } else if (m_ExplicitHeight > 0.0f) {
        h = std::min(allottedRect.height, m_ExplicitHeight * uiScale);
    }
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    m_Geometry = Rect{ allottedRect.x, y, allottedRect.width, h };
}

void VerticalDivider::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ColorToken::Separator));
}

} // namespace we::runtime::kindui
