#include "PlaceActors/PlaceActorsRecentlyUsedManager.h"

#include "PlaceActors/PlaceActorsCatalog.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "EditorToolsRegistry.h"

namespace we::programs::editor {

PlaceActorsRecentlyUsedManager& PlaceActorsRecentlyUsedManager::Get() {
    static PlaceActorsRecentlyUsedManager instance;
    return instance;
}

const std::vector<std::string>& PlaceActorsRecentlyUsedManager::GetRecentToolIds() const {
    return EditorToolsRegistry::Get().GetRecentToolIds();
}

std::vector<PlaceActorsItemData> PlaceActorsRecentlyUsedManager::CollectRecentItems(
    const std::string& searchQuery) const {
    std::vector<PlaceActorsItemData> items;
    auto& catalog = PlaceActorsCatalog::Get();
    for (const std::string& toolId : GetRecentToolIds()) {
        if (const PlaceActorsItemData* item = catalog.FindItem(toolId)) {
            if (!PlaceActorsSearch::FilterItems({ *item }, searchQuery, "All").empty()) {
                items.push_back(*item);
            }
        }
    }
    return items;
}

} // namespace we::programs::editor
