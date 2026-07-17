#include "PlaceActors/PlaceActorsFavoritesManager.h"

#include "PlaceActors/PlaceActorsCatalog.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"

namespace we::programs::editor {
using ::we::editor::toolspanel::EditorToolsRegistry;
using ::we::editor::toolspanel::EditorToolAction;

PlaceActorsFavoritesManager& PlaceActorsFavoritesManager::Get() {
    static PlaceActorsFavoritesManager instance;
    return instance;
}

bool PlaceActorsFavoritesManager::IsFavorite(const std::string& toolId) const {
    return EditorToolsRegistry::Get().IsFavorite(toolId);
}

void PlaceActorsFavoritesManager::Toggle(const std::string& toolId) {
    EditorToolsRegistry::Get().ToggleFavorite(toolId);
}

void PlaceActorsFavoritesManager::SetFavorite(const std::string& toolId, bool favorite) {
    EditorToolsRegistry::Get().SetFavorite(toolId, favorite);
}

void PlaceActorsFavoritesManager::Load(const std::unordered_map<std::string, bool>& favorites) {
    EditorToolsRegistry::Get().LoadFavorites(favorites);
}

std::vector<PlaceActorsItemData> PlaceActorsFavoritesManager::CollectFavoriteItems(
    const std::string& searchQuery) const {
    std::vector<PlaceActorsItemData> items;
    auto& catalog = PlaceActorsCatalog::Get();
    for (const EditorToolAction* tool : EditorToolsRegistry::Get().GetFavoriteTools("Actors")) {
        if (const PlaceActorsItemData* item = catalog.FindItem(tool->id)) {
            if (PlaceActorsSearch::FilterItems({ *item }, searchQuery, "All").empty()) {
                continue;
            }
            items.push_back(*item);
        }
    }
    return items;
}

const std::unordered_map<std::string, bool>& PlaceActorsFavoritesManager::GetStates() const {
    return EditorToolsRegistry::Get().GetFavoriteStates();
}

} // namespace we::programs::editor
