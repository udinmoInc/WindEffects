#pragma once

#include "KindUI/Core/WidgetVariant.h"

namespace we::programs::welauncher::projects {

/// Style class tokens for the Projects page (resolved via theme, no inline colors).
struct Styles {
    static constexpr const char* PageRoot = "launcher.projects.page";
    static constexpr const char* Chrome = "launcher.projects.chrome";
    static constexpr const char* TableHost = "launcher.projects.table";
    static constexpr const char* Toolbar = "launcher.projects.toolbar";
    static constexpr const char* SearchRow = "launcher.projects.search";
    static constexpr const char* TableHeader = "launcher.projects.header";
    static constexpr const char* TableBody = "launcher.projects.body";
    static constexpr const char* EmptyState = "launcher.projects.empty";
};

inline we::runtime::kindui::WidgetVariant PrimaryAction() {
    return we::runtime::kindui::WidgetVariant::Primary;
}

inline we::runtime::kindui::WidgetVariant SecondaryAction() {
    return we::runtime::kindui::WidgetVariant::Secondary;
}

} // namespace we::programs::welauncher::projects
