#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::runtime::kindui {

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Point& position) {
    if (!icon.IsValid()) {
        return;
    }
    const float drawSize = static_cast<float>(icon.sizePx);
    const Rect drawRect{ position.x, position.y, drawSize, drawSize };
    context.DrawWindIcon(icon, drawRect);
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, uint32_t displayPx) {
    if (!icon.IsValid()) {
        return;
    }
    const uint32_t drawPx = displayPx > 0 ? displayPx : icon.sizePx;
    const Rect drawRect = IconMetrics::PlaceGlyphCentered(controlBounds, drawPx);
    context.DrawWindIcon(icon, drawRect);
}

} // namespace we::runtime::kindui
