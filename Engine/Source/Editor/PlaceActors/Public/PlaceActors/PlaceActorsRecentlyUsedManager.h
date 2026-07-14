#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include <string>
#include <vector>

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsRecentlyUsedManager {
public:
    static PlaceActorsRecentlyUsedManager& Get();

    [[nodiscard]] const std::vector<std::string>& GetRecentToolIds() const;

    [[nodiscard]] std::vector<PlaceActorsItemData> CollectRecentItems(
        const std::string& searchQuery) const;
};

} // namespace we::programs::editor
