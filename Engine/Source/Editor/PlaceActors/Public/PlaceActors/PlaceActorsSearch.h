#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include "Core/PaintContext.h"
#include "Core/Geometry.h"
#include <string>
#include <utility>
#include <vector>

namespace we::programs::editor {

struct PlaceActorsSearchMatch {
    size_t start = 0;
    size_t length = 0;
};

class PlaceActorsSearch {
public:
    static std::vector<PlaceActorsItemData> FilterItems(
        const std::vector<PlaceActorsItemData>& items,
        const std::string& query,
        const std::string& categoryFilter);

    static void SortItems(std::vector<PlaceActorsItemData>& items, PlaceActorsSortMode mode);

    static PlaceActorsSearchMatch FindLabelMatch(const std::string& label, const std::string& query);

    static void PaintHighlightedLabel(
        WindEffects::Editor::UI::PaintContext& context,
        const std::string& label,
        const WindEffects::Editor::UI::Point& position,
        float fontSize,
        const std::string& query,
        const WindEffects::Editor::UI::Color& normalColor,
        const WindEffects::Editor::UI::Color& highlightColor,
        float maxWidth = 0.0f);
};

} // namespace we::programs::editor
