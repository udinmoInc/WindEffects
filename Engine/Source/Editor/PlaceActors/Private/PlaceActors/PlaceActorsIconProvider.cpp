// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsIconProvider
// Internal implementation for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "PlaceActors/PlaceActorsIconProvider.h"

#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include <KindUI/EditorUI.h>
namespace we::programs::editor {
using ::we::runtime::kindui::kWindIconNone;
using ::we::runtime::kindui::WindIconRef;
using ::we::editor::toolspanel::EditorToolsRegistry;

PlaceActorsIconProvider& PlaceActorsIconProvider::Get() {
    static PlaceActorsIconProvider instance;
    return instance;
}

WindIconRef PlaceActorsIconProvider::ResolveChromeIcon(const PlaceActorsItemData& item) const {
    return ResolvePreviewIcon(item.toolId);
}

WindIconRef PlaceActorsIconProvider::ResolvePreviewIcon(const PlaceActorsItemData& item) const {
    return ResolvePreviewIcon(item.toolId);
}

WindIconRef PlaceActorsIconProvider::ResolvePreviewIcon(const std::string& toolId) const {
    if (const auto* tool = EditorToolsRegistry::Get().FindTool(toolId)) {
        return tool->icon;
    }
    return kWindIconNone;
}

bool PlaceActorsIconProvider::HasPreviewIcon(const std::string& toolId) const {
    return ResolvePreviewIcon(toolId).IsValid();
}

} // namespace we::programs::editor
