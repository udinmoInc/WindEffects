#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

namespace we::runtime::kindui {

class PaintContext;

class KINDUI_API IconPainter {
public:
    /// Draw a WindIcon centered in controlBounds at its authored sizePx.
    /// displayPx is ignored (kept for call-site compatibility) — never rescale.
    static void Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, uint32_t displayPx = 0);
    static void Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, const Color& tint);

    /// Draw a WindIcon at an explicit position using its authored pixel size.
    static void Draw(PaintContext& context, WindIconRef icon, const Point& position);
    static void Draw(PaintContext& context, WindIconRef icon, const Point& position, const Color& tint);
};

} // namespace we::runtime::kindui
