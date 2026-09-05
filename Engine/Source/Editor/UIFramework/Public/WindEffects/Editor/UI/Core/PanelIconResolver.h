#pragma once

#include "KindUI/Core/WindIcon.h"

#include <string_view>

namespace we::editor::services {

// Explicit dock-tab icon assignments. Unlisted panels render with no tab icon.
inline we::runtime::kindui::WindIconRef ResolvePanelTabIcon(std::string_view panelId) {
    using namespace we::runtime::kindui;
    if (panelId == "content-browser") {
        return WindIcons::FolderSearch16;
    }
    if (panelId == "output-log") {
        return WindIcons::Console16;
    }
    if (panelId == "terminal") {
        return WindIcons::Console16;
    }
    if (panelId == "viewport") {
        return WindIcons::ToolbarCamera16;
    }
    if (panelId == "outliner") {
        return WindIcons::Outliner16;
    }
    if (panelId == "details") {
        return WindIcons::AdjustHorizon16;
    }
    if (panelId == "tools-panel") {
        return WindIcons::Wrench16;
    }
    if (panelId == "place-actors") {
        return WindIcons::CirclePlus16;
    }
    return kWindIconNone;
}

} // namespace we::editor::services
