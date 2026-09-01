#include "KindUI/Core/Widgets/VerticalDivider.h"

#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

Size VerticalDivider::Measure(const Size& availableSize) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_DesiredSize = Size{
        ds::Chrome::SeparationGapWide() * uiScale,
        availableSize.height > 0.0f
            ? availableSize.height
            : ResolveMetric(MetricToken::ToolbarSeparatorHeight) * uiScale
    };
    return m_DesiredSize;
}

void VerticalDivider::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void VerticalDivider::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float sepH = ResolveMetric(MetricToken::ToolbarSeparatorHeight) * uiScale;
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float centerX = m_Geometry.x + m_Geometry.width * 0.5f;
    ControlChrome::PaintVerticalSeparator(
        context,
        centerX,
        centerY - sepH * 0.5f,
        centerY + sepH * 0.5f,
        ResolveMetric(MetricToken::BorderWidth),
        ColorToken::Separator);
}

} // namespace we::runtime::kindui
