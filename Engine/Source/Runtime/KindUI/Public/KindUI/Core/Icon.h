#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

namespace we::runtime::kindui {

class PaintContext;

class KINDUI_API IconPainter {
public:
    /// Draw a WindIcon centered in bounds at its authored pixel size.
    static void Draw(PaintContext& context, WindIconRef icon, const Rect& bounds);

    /// Draw a WindIcon at an explicit position using its authored pixel size.
    static void Draw(PaintContext& context, WindIconRef icon, const Point& position);
};

} // namespace we::runtime::kindui
