#include "UI/Pages/LauncherPages.h"

#include "KindUI/Declarative/UI.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Icon.h"
#include "UI/Widgets/LauncherControls.h"
#include "UI/Widgets/ManagerViews.h"
#include "UI/Widgets/SkeletonViews.h"

namespace we::programs::welauncher {
namespace UI = we::runtime::kindui::UI;
using namespace we::runtime::kindui;

namespace {

we::runtime::kindui::Element PageBody(std::vector<we::runtime::kindui::Element> children) {
    const float s = LScale();
    return UI::Column(std::move(children));
}

we::runtime::kindui::Element LoadingRows(int count) {
    std::vector<we::runtime::kindui::Element> rows;
    rows.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        rows.push_back(UI::Host([] { return std::make_shared<SkeletonCard>(SkeletonKind::ListRow); }));
    }
    rows.push_back(UI::Spacer());
    return UI::Column(std::move(rows));
}

} // namespace

we::runtime::kindui::Element BuildPageChrome(const PageChromeModel& model) {
    const float s = LScale();

    std::vector<we::runtime::kindui::Element> headerChildren;
    headerChildren.push_back(UI::Section(model.title, model.subtitle));
    headerChildren.push_back(UI::Spacer());
    for (auto& action : model.toolbarActions) {
        headerChildren.push_back(std::move(action));
    }

    auto header = UI::Row(std::move(headerChildren));

    we::runtime::kindui::Element searchRow = UI::Row({
        UI::Spacer(),
        UI::Host([model]() {
            auto search = std::make_shared<CompactSearchField>(model.searchPlaceholder);
            search->SetText(model.searchQuery);
            if (model.onSearchChanged) {
                search->SetOnChanged(model.onSearchChanged);
            }
            return search;
        }),
    });

    return UI::Column({
        UI::Padding(header, kLauncherContentPadX * s, kLauncherContentPadTop * s, kLauncherContentPadX * s, 0.0f),
        UI::Host([s]() { return std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s); }),
        UI::Padding(searchRow, kLauncherContentPadX * s, 0.0f, kLauncherContentPadX * s, 0.0f),
        UI::Host([s]() { return std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s); }),
    });
}

we::runtime::kindui::Element BuildLearnPage(const LearnPageModel& model) {
    const float s = LScale();

    if (model.chrome.loading) {
        return UI::Column({
            BuildPageChrome(model.chrome),
            UI::Fill(PageBody({ LoadingRows(8) })),
        });
    }

    std::vector<we::runtime::kindui::Element> tableChildren;
    tableChildren.push_back(UI::Host([] {
        return std::make_shared<SimpleColumnHeader>(
            std::vector<std::string>{ "NAME", "TYPE", "DETAIL" });
    }));

    if (model.showEmpty) {
        const std::string emptyTitle = model.emptyTitle;
        const std::string emptySubtitle = model.emptySubtitle;
        auto onClearSearch = model.onClearSearch;
        tableChildren.push_back(UI::Fill(UI::Host([emptyTitle, emptySubtitle, onClearSearch]() {
            auto empty = std::make_shared<EmptyStatePanel>(
                emptyTitle,
                emptySubtitle,
                onClearSearch ? Icons::SearchName : Icons::MediaPlayName);
            if (onClearSearch) {
                empty->SetPrimaryAction("Clear Search", Icons::RefreshName, onClearSearch);
            }
            return empty;
        })));
    } else {
        std::vector<we::runtime::kindui::Element> rows;
        rows.reserve(model.entries.size() + 1);
        for (std::size_t i = 0; i < model.entries.size(); ++i) {
            const auto& entry = model.entries[i];
            rows.push_back(UI::Id(UI::Host([entry]() {
                return std::make_shared<LibraryPackageRow>(
                    entry.kind,
                    entry.name,
                    entry.detail,
                    entry.icon.c_str(),
                    entry.onClick);
            }), "learn-row-" + std::to_string(i)));
        }
        rows.push_back(UI::Spacer());

        tableChildren.push_back(UI::Id(UI::Scroll(std::move(rows)), "learn-scroll"));
    }

    return UI::Column({
        BuildPageChrome(model.chrome),
        UI::Fill(UI::Column(std::move(tableChildren))),
    });
}

we::runtime::kindui::Element BuildEnginePage(const EnginePageModel& model) {
    if (model.chrome.loading) {
        return UI::Column({
            BuildPageChrome(model.chrome),
            UI::Fill(PageBody({ LoadingRows(4) })),
        });
    }

    std::vector<we::runtime::kindui::Element> tableChildren;
    tableChildren.push_back(UI::Host([] {
        return std::make_shared<SimpleColumnHeader>(
            std::vector<std::string>{ "VERSION", "BUILD", "PATH", "SDK" });
    }));

    if (model.showEmpty) {
        const std::string emptyTitle = model.emptyTitle;
        const std::string emptySubtitle = model.emptySubtitle;
        auto onOpenProjects = model.onOpenProjects;
        auto onRefresh = model.onRefresh;
        auto onClearSearch = model.onClearSearch;
        tableChildren.push_back(UI::Fill(UI::Host([emptyTitle, emptySubtitle, onOpenProjects, onRefresh, onClearSearch]() {
            auto empty = std::make_shared<EmptyStatePanel>(
                emptyTitle,
                emptySubtitle,
                Icons::BuildName);
            if (onOpenProjects) {
                empty->SetPrimaryAction("Open Projects", Icons::OpenFolderName, onOpenProjects);
            }
            if (onRefresh) {
                empty->SetSecondaryAction("Refresh", Icons::RefreshName, onRefresh);
            }
            if (onClearSearch) {
                empty->SetPrimaryAction("Clear Search", Icons::RefreshName, onClearSearch);
            }
            return empty;
        })));
    } else {
        std::vector<we::runtime::kindui::Element> rows = model.rows;
        rows.push_back(UI::Spacer());
        tableChildren.push_back(UI::Id(UI::Scroll(std::move(rows)), "engine-scroll"));
    }

    return UI::Column({
        BuildPageChrome(model.chrome),
        UI::Fill(UI::Column(std::move(tableChildren))),
    });
}

} // namespace we::programs::welauncher
