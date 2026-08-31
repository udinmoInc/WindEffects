#include "KindUI/Rendering/IconMetrics.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui::IconMetrics {

float SnapPx(float value) {
    return std::floor(value + 0.5f);
}

float IconButtonHitPx(float uiScale) {
    return ResolveMetric(MetricToken::IconButtonSize) * std::max(1.0f, uiScale);
}

float IconContentPaddingPx(float uiScale) {
    const float hit = IconButtonHitPx(uiScale);
    const float tier = ResolveMetric(MetricToken::IconSizeToolbar);
    return std::max(0.0f, (hit - tier) * 0.5f);
}

Rect PlaceGlyphCentered(const Rect& controlBounds, uint32_t sizePx) {
    const float drawSize = static_cast<float>(sizePx);
    const float x = SnapPx(controlBounds.x + (controlBounds.width - drawSize) * 0.5f);
    const float y = SnapPx(controlBounds.y + (controlBounds.height - drawSize) * 0.5f);
    return Rect{ x, y, drawSize, drawSize };
}

Rect PlaceGlyphCentered(const Rect& controlBounds, float logicalSizePx) {
    const uint32_t rounded = static_cast<uint32_t>(std::lround(std::max(0.0f, logicalSizePx)));
    return PlaceGlyphCentered(controlBounds, std::max(1u, rounded));
}

Rect CompactGlyphBand(const Rect& controlBounds, float x) {
    const float tier = static_cast<float>(kCompactGlyphPx);
    return Rect{ x, controlBounds.y, tier, controlBounds.height };
}

} // namespace we::runtime::kindui::IconMetrics
