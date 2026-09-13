// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsCategory
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"

#include <KindUI/EditorUI.h>
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
