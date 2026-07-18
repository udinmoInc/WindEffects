#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsFavoritesManager {
public:
    static PlaceActorsFavoritesManager& Get();

    [[nodiscard]] bool IsFavorite(const std::string& toolId) const;
    void Toggle(const std::string& toolId);
    void SetFavorite(const std::string& toolId, bool favorite);
    void Load(const std::unordered_map<std::string, bool>& favorites);

    [[nodiscard]] std::vector<PlaceActorsItemData> CollectFavoriteItems(
        const std::string& searchQuery) const;

    [[nodiscard]] const std::unordered_map<std::string, bool>& GetStates() const;
};

} // namespace we::programs::editor
