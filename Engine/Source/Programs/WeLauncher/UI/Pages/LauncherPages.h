#pragma once

#include "KindUI/Declarative/Element.h"
#include "UI/LauncherHelpers.h"

#include <functional>
#include <string>
#include <vector>

namespace we::programs::welauncher {

/// Shared page chrome: title, subtitle, optional toolbar actions, optional search.
struct PageChromeModel {
    std::string title;
    std::string subtitle;
    std::string searchPlaceholder;
    std::string searchQuery;
    bool loading = false;
    std::function<void(const std::string&)> onSearchChanged;
    std::vector<we::runtime::kindui::Element> toolbarActions;
};

[[nodiscard]] we::runtime::kindui::Element BuildPageChrome(const PageChromeModel& model);

struct LearnEntryModel {
    std::string kind;
    std::string name;
    std::string detail;
    std::string icon;
    std::function<void()> onClick;
};

struct LearnPageModel {
    PageChromeModel chrome;
    std::vector<LearnEntryModel> entries;
    bool showEmpty = false;
    std::string emptyTitle;
    std::string emptySubtitle;
    std::function<void()> onClearSearch;
};

[[nodiscard]] we::runtime::kindui::Element BuildLearnPage(const LearnPageModel& model);

struct EnginePageModel {
    PageChromeModel chrome;
    bool hasEngine = false;
    std::vector<we::runtime::kindui::Element> rows;
    bool showEmpty = false;
    std::string emptyTitle;
    std::string emptySubtitle;
    std::function<void()> onClearSearch;
    std::function<void()> onOpenProjects;
    std::function<void()> onRefresh;
};

[[nodiscard]] we::runtime::kindui::Element BuildEnginePage(const EnginePageModel& model);

} // namespace we::programs::welauncher
