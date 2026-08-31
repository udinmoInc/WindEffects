#pragma once

#include "PlaceActors/Export.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsCategory {
public:
    static float MeasureHeaderHeight(float configuredHeight);

    static void PaintHeader(we::runtime::kindui::PaintContext& context,
                            const we::runtime::kindui::Rect& bounds,
                            const std::string& label,
                            we::runtime::kindui::WindIconRef icon,
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
