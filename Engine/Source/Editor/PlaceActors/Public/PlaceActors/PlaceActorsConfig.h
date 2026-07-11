#pragma once

#include "PlaceActors/PlaceActorsTypes.h"
#include <string>

namespace we::programs::editor {

class PlaceActorsConfig {
public:
    static PlaceActorsConfig& Get();

    void EnsureLoaded();

    PlaceActorsViewMode defaultView = PlaceActorsViewMode::List;
    float iconSize = 14.0f;
    float cardSize = 72.0f;
    bool showDescriptions = true;
    bool enableAnimations = true;
    bool rememberCategoryState = true;
    bool rememberSearchHistory = true;
    bool showRecent = true;
    bool showFavorites = true;
    int gridColumns = 3;
    float listRowHeight = 22.0f;
    float categoryHeaderHeight = 22.0f;

private:
    PlaceActorsConfig() = default;
    void Load();

    bool m_Loaded = false;
};

} // namespace we::programs::editor
