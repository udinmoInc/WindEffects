#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

namespace we::runtime::kindui {

class PaintContext;

class KINDUI_API IconPainter {
public:
    /// Draw a WindIcon centered in controlBounds.
    /// displayPx selects on-screen size (loads nearest authored tier); 0 uses icon.sizePx.
    static void Draw(PaintContext& context, WindIconRef icon, const Rect& controlBounds, uint32_t displayPx = 0);

    /// Draw a WindIcon at an explicit position using its authored pixel size.
    static void Draw(PaintContext& context, WindIconRef icon, const Point& position);
};

} // namespace we::runtime::kindui
