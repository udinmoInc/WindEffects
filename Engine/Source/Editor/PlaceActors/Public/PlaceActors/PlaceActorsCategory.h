#pragma once

#include "Core/PaintContext.h"
#include "Core/Geometry.h"
#include <string>

namespace we::programs::editor {

class PlaceActorsCategory {
public:
    static float MeasureHeaderHeight(float configuredHeight);

    static void PaintHeader(WindEffects::Editor::UI::PaintContext& context,
                            const WindEffects::Editor::UI::Rect& bounds,
                            const std::string& label,
                            const std::string& iconName,
                            bool expanded,
                            float hoverAnim);
};

} // namespace we::programs::editor
