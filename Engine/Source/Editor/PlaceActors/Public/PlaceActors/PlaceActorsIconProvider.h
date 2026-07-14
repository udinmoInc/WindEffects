#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include <string>

namespace we::programs::editor {

// Resolves actor icons: Lucide glyphs for chrome/list, atlas 3D thumbnails for previews.
class PLACEACTORS_API PlaceActorsIconProvider {
public:
    static PlaceActorsIconProvider& Get();

    // Lucide (or registry) icon used in list rows and category chrome.
    [[nodiscard]] std::string ResolveChromeIcon(const PlaceActorsItemData& item) const;

    // Atlas id for thumbnail preview (3dcube / 3dsphere / …), or empty if none.
    [[nodiscard]] std::string ResolvePreviewIcon(const PlaceActorsItemData& item) const;
    [[nodiscard]] std::string ResolvePreviewIcon(const std::string& toolId) const;

    [[nodiscard]] bool HasPreviewIcon(const std::string& toolId) const;
};

} // namespace we::programs::editor
