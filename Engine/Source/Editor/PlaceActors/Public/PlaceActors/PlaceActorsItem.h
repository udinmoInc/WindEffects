#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include "Core/PaintContext.h"
#include "Core/Geometry.h"
#include <string>

namespace we::programs::editor {

struct PlaceActorsItemMetrics {
    float iconSize = 56.0f;
    float cardSize = 88.0f;
    float listRowHeight = 44.0f;
    float cornerRadius = 7.0f;
};

class PlaceActorsItem {
public:
    static WindEffects::Editor::UI::Size MeasureGrid(const PlaceActorsItemMetrics& metrics);
    static WindEffects::Editor::UI::Size MeasureList(const PlaceActorsItemMetrics& metrics);

    static void PaintGrid(WindEffects::Editor::UI::PaintContext& context,
                          const WindEffects::Editor::UI::Rect& bounds,
                          const PlaceActorsItemData& item,
                          const PlaceActorsItemMetrics& metrics,
                          float hoverAnim,
                          float pressAnim,
                          bool selected,
                          bool favorite);

    static void PaintList(WindEffects::Editor::UI::PaintContext& context,
                          const WindEffects::Editor::UI::Rect& bounds,
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
