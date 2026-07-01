#include "PlaceActors/PlaceActorsSearch.h"

#include <algorithm>
#include <cctype>

namespace we::programs::editor {

namespace {

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool ContainsInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

bool MatchesItem(const PlaceActorsItemData& item, const std::string& query) {
    if (query.empty()) return true;
    if (ContainsInsensitive(item.label, query)) return true;
    if (ContainsInsensitive(item.categoryLabel, query)) return true;
    for (const auto& tag : item.tags) {
        if (ContainsInsensitive(tag, query)) return true;
    }
    for (const auto& alias : item.aliases) {
        if (ContainsInsensitive(alias, query)) return true;
    }
    return false;
}

} // namespace

std::vector<PlaceActorsItemData> PlaceActorsSearch::FilterItems(
    const std::vector<PlaceActorsItemData>& items,
    const std::string& query,
    const std::string& categoryFilter)
{
    std::vector<PlaceActorsItemData> filtered;
    filtered.reserve(items.size());
    for (const auto& item : items) {
        if (categoryFilter != "All" && item.categoryLabel != categoryFilter) {
            continue;
        }
        if (!MatchesItem(item, query)) {
            continue;
        }
        filtered.push_back(item);
    }
    return filtered;
}

void PlaceActorsSearch::SortItems(std::vector<PlaceActorsItemData>& items, PlaceActorsSortMode mode) {
    switch (mode) {
    case PlaceActorsSortMode::Category:
        std::sort(items.begin(), items.end(), [](const PlaceActorsItemData& a, const PlaceActorsItemData& b) {
            if (a.categoryLabel != b.categoryLabel) return a.categoryLabel < b.categoryLabel;
            return a.label < b.label;
        });
        break;
    case PlaceActorsSortMode::Recent:
        break;
    case PlaceActorsSortMode::Name:
    default:
        std::sort(items.begin(), items.end(), [](const PlaceActorsItemData& a, const PlaceActorsItemData& b) {
            return a.label < b.label;
        });
        break;
    }
}

} // namespace we::programs::editor
