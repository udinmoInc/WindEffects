// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsFavoritesManager
// Internal implementation for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
