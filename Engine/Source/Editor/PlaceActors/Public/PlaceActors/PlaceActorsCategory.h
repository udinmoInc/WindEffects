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
                            float hoverAnim,
                            float expandAnim,
                            bool isFavoritesSection,
                            bool showChevron);

    static void PaintSectionBackground(WindEffects::Editor::UI::PaintContext& context,
                                       const WindEffects::Editor::UI::Rect& bounds);

    static void PaintEmptyState(WindEffects::Editor::UI::PaintContext& context,
                                const WindEffects::Editor::UI::Rect& bounds,
                                const std::string& message);
};

} // namespace we::programs::editor
