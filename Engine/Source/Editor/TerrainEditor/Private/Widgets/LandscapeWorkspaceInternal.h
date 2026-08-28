#pragma once

#include "TerrainEditor/ILandscapeEditor.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include <memory>
#include <string>

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
