// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsResponsiveGrid
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"
#include "KindUI/Core/Geometry.h"
#include <algorithm>
#include <cmath>

namespace we::programs::editor {

struct PlaceActorsGridMetrics {
    float minCardWidth = 76.0f;
    float maxCardWidth = 120.0f;
    float cardGap = 8.0f;
    float contentPad = 8.0f;
    float scrollbarReserve = 12.0f;
    float labelHeight = 18.0f;
    float labelGap = 4.0f;
};

struct PlaceActorsGridLayout {
    int columns = 1;
    float cardWidth = 76.0f;
    float previewSize = 76.0f;
    float cardHeight = 98.0f;
    float cardGap = 8.0f;
    float contentPad = 8.0f;
    float scrollbarReserve = 12.0f;
};

class PLACEACTORS_API PlaceActorsResponsiveGrid {
public:
    // Automatically picks 1..N columns from available width. No manual column config needed.
    static PlaceActorsGridLayout Compute(float viewportWidth, const PlaceActorsGridMetrics& metrics = {});

    static we::runtime::kindui::Rect CardRect(
        const PlaceActorsGridLayout& layout,
        float originX,
        float originY,
        int index);

    static we::runtime::kindui::Rect PreviewRect(
        const PlaceActorsGridLayout& layout,
        const we::runtime::kindui::Rect& cardRect);

    static we::runtime::kindui::Rect LabelRect(
        const PlaceActorsGridLayout& layout,
        const we::runtime::kindui::Rect& cardRect);

    static float ContentHeight(const PlaceActorsGridLayout& layout, int itemCount);
};

} // namespace we::programs::editor
