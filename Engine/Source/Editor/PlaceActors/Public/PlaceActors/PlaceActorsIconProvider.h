#pragma once

#include "PlaceActors/Export.h"
#include "PlaceActors/PlaceActorsTypes.h"
#include "KindUI/Core/WindIcon.h"

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsIconProvider {
public:
    static PlaceActorsIconProvider& Get();

    [[nodiscard]] we::runtime::kindui::WindIconRef ResolveChromeIcon(const PlaceActorsItemData& item) const;
    [[nodiscard]] we::runtime::kindui::WindIconRef ResolvePreviewIcon(const PlaceActorsItemData& item) const;
    [[nodiscard]] we::runtime::kindui::WindIconRef ResolvePreviewIcon(const std::string& toolId) const;
    [[nodiscard]] bool HasPreviewIcon(const std::string& toolId) const;
};

} // namespace we::programs::editor
