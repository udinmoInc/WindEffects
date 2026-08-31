#pragma once

#include "KindUI/Core/WindIcon.h"

#include <string_view>

namespace we::editor::services {

// Explicit dock-tab icon assignments. Unlisted panels render with no tab icon.
inline we::runtime::kindui::WindIconRef ResolvePanelTabIcon(std::string_view panelId) {
    using namespace we::runtime::kindui;
    if (panelId == "content-browser") {
        return kWindIconNone;
    }
    if (panelId == "output-log") {
        return kWindIconNone;
    }
    if (panelId == "terminal") {
        return kWindIconNone;
    }
    if (panelId == "viewport") {
        return kWindIconNone;
    }
    if (panelId == "outliner") {
        return kWindIconNone;
    }
    if (panelId == "details") {
        return kWindIconNone;
    }
    if (panelId == "tools-panel") {
        return kWindIconNone;
    }
    if (panelId == "place-actors") {
        return kWindIconNone;
    }
    return kWindIconNone;
}

} // namespace we::editor::services
