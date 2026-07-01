#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include "Core/PaintContext.hpp"
#include "Core/Geometry.hpp"

namespace we::programs::editor {

struct PlaceActorsItemMetrics {
    float iconSize = 56.0f;
    float cardSize = 88.0f;
    float listRowHeight = 44.0f;
    float cornerRadius = 7.0f;
};

class PlaceActorsItem {
public:
    static we::UI::Size MeasureGrid(const PlaceActorsItemMetrics& metrics);
    static we::UI::Size MeasureList(const PlaceActorsItemMetrics& metrics);

    static void PaintGrid(we::UI::PaintContext& context,
                          const we::UI::Rect& bounds,
                          const PlaceActorsItemData& item,
                          const PlaceActorsItemMetrics& metrics,
                          float hoverAnim,
                          float pressAnim,
                          bool selected,
                          bool favorite);

    static void PaintList(we::UI::PaintContext& context,
                          const we::UI::Rect& bounds,
                          const PlaceActorsItemData& item,
                          const PlaceActorsItemMetrics& metrics,
                          float hoverAnim,
                          float pressAnim,
                          bool selected,
                          bool favorite);
};

} // namespace we::programs::editor
