#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"

namespace we::runtime::kindui {

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Point& position) {
    Draw(context, icon, position, Color::White());
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Point& position, const Color& tint) {
    if (!icon.IsValid()) {
        return;
    }
    const float drawSize = static_cast<float>(icon.sizePx);
    const Rect drawRect{ position.x, position.y, drawSize, drawSize };
    context.DrawWindIcon(icon, drawRect, tint);
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, uint32_t displayPx) {
    if (!icon.IsValid()) {
        return;
    }
    const float targetPx = displayPx > 0
        ? static_cast<float>(displayPx)
        : (icon.sizePx > 0
            ? static_cast<float>(icon.sizePx)
            : ((controlBounds.width > 0.0f && controlBounds.height > 0.0f)
                ? std::min(controlBounds.width, controlBounds.height)
                : 16.0f));
    const Rect drawRect = IconMetrics::PlaceGlyphCentered(controlBounds, targetPx);
    context.DrawWindIcon(icon, drawRect, Color::White());
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, const Color& tint) {
    if (!icon.IsValid()) {
        return;
    }
    const float targetPx = (icon.sizePx > 0)
        ? static_cast<float>(icon.sizePx)
        : ((controlBounds.width > 0.0f && controlBounds.height > 0.0f)
            ? std::min(controlBounds.width, controlBounds.height)
            : 16.0f);
    const Rect drawRect = IconMetrics::PlaceGlyphCentered(controlBounds, targetPx);
    context.DrawWindIcon(icon, drawRect, tint);
}

} // namespace we::runtime::kindui
