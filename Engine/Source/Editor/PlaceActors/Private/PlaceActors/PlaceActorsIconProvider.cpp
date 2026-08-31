#include "PlaceActors/PlaceActorsIconProvider.h"

#include "KindUI/Core/WindIcon.h"

namespace we::programs::editor {
using ::we::runtime::kindui::kWindIconNone;
using ::we::runtime::kindui::WindIconRef;

PlaceActorsIconProvider& PlaceActorsIconProvider::Get() {
    static PlaceActorsIconProvider instance;
    return instance;
}

WindIconRef PlaceActorsIconProvider::ResolveChromeIcon(const PlaceActorsItemData& item) const {
    (void)item;
    return kWindIconNone;
}

WindIconRef PlaceActorsIconProvider::ResolvePreviewIcon(const PlaceActorsItemData& item) const {
    return ResolvePreviewIcon(item.toolId);
}

WindIconRef PlaceActorsIconProvider::ResolvePreviewIcon(const std::string& toolId) const {
    (void)toolId;
    return kWindIconNone;
}

bool PlaceActorsIconProvider::HasPreviewIcon(const std::string& toolId) const {
    return ResolvePreviewIcon(toolId).IsValid();
}

} // namespace we::programs::editor
