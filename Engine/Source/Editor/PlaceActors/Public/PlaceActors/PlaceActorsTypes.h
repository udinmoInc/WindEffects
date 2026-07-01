#pragma once

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
    std::string iconName;
    std::string description;
    std::vector<std::string> tags;
    std::vector<std::string> aliases;
    int sortOrder = 0;
    bool favoritable = true;
};

struct PlaceActorsCategoryData {
    std::string id;
    std::string label;
    std::string iconName;
    int sortOrder = 0;
    bool defaultExpanded = true;
    std::vector<PlaceActorsItemData> items;
};

} // namespace we::programs::editor
