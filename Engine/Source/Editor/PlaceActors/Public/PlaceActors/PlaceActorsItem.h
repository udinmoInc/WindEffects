// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsItem
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"

#include "PlaceActors/PlaceActorsTypes.h"
#include <KindUI/EditorUI.h>
#include <string>

namespace we::programs::editor {

struct PlaceActorsItemMetrics {
    float iconSize = 56.0f;
    float cardSize = 88.0f;
    float cardHeight = 108.0f;
    float previewSize = 88.0f;
    float listRowHeight = 44.0f;
    float cornerRadius = 7.0f;
};

class PLACEACTORS_API PlaceActorsItem {
public:
    static we::runtime::kindui::Size MeasureGrid(const PlaceActorsItemMetrics& metrics);
    static we::runtime::kindui::Size MeasureList(const PlaceActorsItemMetrics& metrics);

    static void PaintGrid(we::runtime::kindui::PaintContext& context,
                          const we::runtime::kindui::Rect& bounds,
                          const PlaceActorsItemData& item,
                          const PlaceActorsItemMetrics& metrics,
                          float hoverAnim,
                          float pressAnim,
                          bool selected,
                          bool favorite);

    static void PaintList(we::runtime::kindui::PaintContext& context,
                          const we::runtime::kindui::Rect& bounds,
                          const PlaceActorsItemData& item,
                          const PlaceActorsItemMetrics& metrics,
                          float hoverAnim,
                          float pressAnim,
                          bool selected,
                          bool favorite,
                          const std::string& searchQuery = {},
                          float revealAnim = 1.0f);
};

} // namespace we::programs::editor
