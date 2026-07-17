#pragma once

#include "KindUI/Core/Icon.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace we::editor::services {
namespace Icons = ::we::runtime::kindui::Icons;

// Canonical dock-tab and panel icon aliases — single registry for shell layout.
inline std::string ResolvePanelTabIconName(std::string_view iconResource) {
    if (iconResource.empty()) {
        return {};
    }

    static const std::unordered_map<std::string, const char*> kPanelTabIcons = {
        {"tools-panel", Icons::LayersName},
        {"viewport", Icons::PerspectiveName},
        {"outliner", Icons::HierarchyName},
        {"details", Icons::PropertiesName},
        {"content-browser", Icons::ContentBrowserName},
        {"output-log", Icons::OutputLogName},
        {"terminal", Icons::TerminalName},
        {"place-actors", Icons::PivotName},
    };

    const std::string key(iconResource);
    if (const auto it = kPanelTabIcons.find(key); it != kPanelTabIcons.end()) {
        return it->second;
    }

    return Icons::ResolveLucideName(key);
}

} // namespace we::editor::services
