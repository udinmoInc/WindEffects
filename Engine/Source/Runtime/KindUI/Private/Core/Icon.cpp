// ==============================================================================
// WindEffects — KindUI — Icon
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>

namespace we::runtime::kindui {

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Point& position) {
    Draw(context, icon, position, Color::White());
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Point& position, const Color& tint) {
    if (!icon.IsValid()) {
        return;
    }
    const float drawSize = static_cast<float>(icon.sizePx);
    const float x = IconMetrics::SnapPx(position.x);
    const float y = IconMetrics::SnapPx(position.y);
    const Rect drawRect{ x, y, drawSize, drawSize };
    context.DrawWindIcon(icon, drawRect, tint);
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, uint32_t displayPx) {
    Draw(context, icon, controlBounds, displayPx, Color::White());
}

void IconPainter::Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, const Color& tint) {
    const uint32_t displayPx = icon.sizePx > 0 ? icon.sizePx : 0u;
    Draw(context, icon, controlBounds, displayPx, tint);
}

void IconPainter::Draw(
    PaintContext& context,
    WindIconRef icon,
    const Rect& controlBounds,
    uint32_t displayPx,
    const Color& tint)
{
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
    context.DrawWindIcon(icon, drawRect, tint);
}

} // namespace we::runtime::kindui
 
