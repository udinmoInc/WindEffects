#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include <string>
#include <vector>

namespace we::programs::editor {

class PlaceActorsSearch {
public:
    static std::vector<PlaceActorsItemData> FilterItems(
        const std::vector<PlaceActorsItemData>& items,
        const std::string& query,
        const std::string& categoryFilter);

    static void SortItems(std::vector<PlaceActorsItemData>& items, PlaceActorsSortMode mode);
};

} // namespace we::programs::editor
