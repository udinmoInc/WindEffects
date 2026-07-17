#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"
#include <string>

namespace we::programs::editor {

class PlaceActorsCategory {
public:
    static float MeasureHeaderHeight(float configuredHeight);

    static void PaintHeader(we::runtime::kindui::PaintContext& context,
                            const we::runtime::kindui::Rect& bounds,
                            const std::string& label,
                            const std::string& iconName,
                            bool expanded,
                            float hoverAnim,
                            float expandAnim,
                            bool isFavoritesSection,
                            bool showChevron);

    static void PaintSectionBackground(we::runtime::kindui::PaintContext& context,
                                       const we::runtime::kindui::Rect& bounds);

    static void PaintEmptyState(we::runtime::kindui::PaintContext& context,
                                const we::runtime::kindui::Rect& bounds,
                                const std::string& message);
};

} // namespace we::programs::editor
