#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Geometry.h"
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

class PlaceActorsItem {
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
