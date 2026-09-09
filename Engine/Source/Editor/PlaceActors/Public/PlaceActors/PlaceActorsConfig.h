// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsConfig
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"

#include "PlaceActors/PlaceActorsTypes.h"
#include <string>

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsConfig {
public:
    static PlaceActorsConfig& Get();

    void EnsureLoaded();

    PlaceActorsViewMode defaultView = PlaceActorsViewMode::Grid;
    float iconSize = 16.0f;
    float cardSize = 88.0f;
    bool showDescriptions = true;
    bool enableAnimations = true;
    bool rememberCategoryState = true;
    bool rememberSearchHistory = true;
    bool showRecent = true;
    bool showFavorites = true;
    int gridColumns = 3;
    float listRowHeight = 0.0f;
    float categoryHeaderHeight = 0.0f;

private:
    PlaceActorsConfig() = default;
    void Load();

    bool m_Loaded = false;
};

} // namespace we::programs::editor
