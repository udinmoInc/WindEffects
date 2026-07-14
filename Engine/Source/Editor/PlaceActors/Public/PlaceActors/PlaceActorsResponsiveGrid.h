#pragma once

#include "PlaceActors/Export.h"
#include "Core/Geometry.h"
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

    static WindEffects::Editor::UI::Rect CardRect(
        const PlaceActorsGridLayout& layout,
        float originX,
        float originY,
        int index);

    static WindEffects::Editor::UI::Rect PreviewRect(
        const PlaceActorsGridLayout& layout,
        const WindEffects::Editor::UI::Rect& cardRect);

    static WindEffects::Editor::UI::Rect LabelRect(
        const PlaceActorsGridLayout& layout,
        const WindEffects::Editor::UI::Rect& cardRect);

    static float ContentHeight(const PlaceActorsGridLayout& layout, int itemCount);
};

} // namespace we::programs::editor
