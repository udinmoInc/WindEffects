// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsRecentlyUsedManager
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
