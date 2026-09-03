#include "KindUI/Core/Widgets/VerticalDivider.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/SurfaceRole.h"
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
    // Full-rect Background gap-cut (connects to workspace chrome).
    context.DrawSurface(m_Geometry, SurfaceRole::Workspace, 0.0f, "VerticalDivider");
}

} // namespace we::runtime::kindui
