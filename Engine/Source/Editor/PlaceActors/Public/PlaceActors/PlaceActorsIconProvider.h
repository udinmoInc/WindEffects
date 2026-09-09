// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsIconProvider
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
