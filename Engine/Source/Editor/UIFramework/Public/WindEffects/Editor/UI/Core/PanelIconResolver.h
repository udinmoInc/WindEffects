#pragma once

#include "KindUI/Core/WindIcon.h"

#include <string_view>

namespace we::editor::services {

// Explicit dock-tab icon assignments. Unlisted panels render with no tab icon.
inline we::runtime::kindui::WindIconRef ResolvePanelTabIcon(std::string_view panelId) {
    using namespace we::runtime::kindui;
    if (panelId == "content-browser" || panelId == "ContentBrowser") {
        return WindIcons::FolderSearch16;
    }
    if (panelId == "output-log" || panelId == "OutputLog" || panelId == "terminal" || panelId == "Terminal") {
        return WindIcons::Console16;
    }
    if (panelId == "viewport" || panelId == "Viewport") {
        return WindIcons::ToolbarCamera16;
    }
    if (panelId == "outliner" || panelId == "WorldOutliner" || panelId == "world-outliner" || panelId == "explorer" || panelId == "Explorer") {
        return WindIcons::Outliner16;
    }
    if (panelId == "details" || panelId == "Details" || panelId == "property-editor" || panelId == "PropertyEditor") {
        return WindIcons::AdjustHorizon16;
    }
    if (panelId == "tools-panel" || panelId == "Tools" || panelId == "ToolsPanel") {
        return WindIcons::Wrench16;
    }
    if (panelId == "place-actors" || panelId == "PlaceActors") {
        return WindIcons::CirclePlus16;
    }
    return kWindIconNone;
}

} // namespace we::editor::services
