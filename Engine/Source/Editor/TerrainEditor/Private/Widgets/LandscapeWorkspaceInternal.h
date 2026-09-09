// ==============================================================================
// WindEffects — TerrainEditor — LandscapeWorkspaceInternal
// UI widget used by the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "LandscapeFormLayout.h"
#include "TerrainEditor/ILandscapeEditor.h"

#include <memory>

namespace we::editor::terrain {

void BuildCreateTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildSculptTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildPaintTab(const std::shared_ptr<we::runtime::kindui::Column>& layout, ILandscapeEditor& editor);
void BuildManageTab(
    const std::shared_ptr<we::runtime::kindui::Column>& layout,
    ILandscapeEditor& editor,
    std::string& importPath,
    std::string& exportPath,
    int& resizeX,
    int& resizeY);

} // namespace we::editor::terrain
