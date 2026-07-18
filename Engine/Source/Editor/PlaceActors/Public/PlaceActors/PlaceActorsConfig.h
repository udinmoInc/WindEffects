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
    float listRowHeight = 32.0f;
    float categoryHeaderHeight = 28.0f;

private:
    PlaceActorsConfig() = default;
    void Load();

    bool m_Loaded = false;
};

} // namespace we::programs::editor
