#pragma once

#include "KindUI/Core/WindIcon.h"

#include <string>
#include <vector>

namespace we::programs::editor {

enum class PlaceActorsViewMode {
    Grid,
    List
};

enum class PlaceActorsSortMode {
    Name,
    Category,
    Recent
};

struct PlaceActorsItemData {
    std::string toolId;
    std::string categoryId;
    std::string categoryLabel;
    std::string label;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    std::string description;
    std::vector<std::string> tags;
    std::vector<std::string> aliases;
    int sortOrder = 0;
    bool favoritable = true;
};

struct PlaceActorsCategoryData {
    std::string id;
    std::string label;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    int sortOrder = 0;
    bool defaultExpanded = true;
    std::vector<PlaceActorsItemData> items;
};

} // namespace we::programs::editor
