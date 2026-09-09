// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsSearch
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"

#include "PlaceActors/PlaceActorsTypes.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"
#include <string>
#include <utility>
#include <vector>

namespace we::programs::editor {

struct PlaceActorsSearchMatch {
    size_t start = 0;
    size_t length = 0;
};

class PLACEACTORS_API PlaceActorsSearch {
public:
    static std::vector<PlaceActorsItemData> FilterItems(
        const std::vector<PlaceActorsItemData>& items,
        const std::string& query,
        const std::string& categoryFilter);

    static void SortItems(std::vector<PlaceActorsItemData>& items, PlaceActorsSortMode mode);

    static PlaceActorsSearchMatch FindLabelMatch(const std::string& label, const std::string& query);

    static void PaintHighlightedLabel(
        we::runtime::kindui::PaintContext& context,
        const std::string& label,
        const we::runtime::kindui::Point& position,
        float fontSize,
        const std::string& query,
        const we::runtime::kindui::Color& normalColor,
        const we::runtime::kindui::Color& highlightColor,
        float maxWidth = 0.0f);
};

} // namespace we::programs::editor
