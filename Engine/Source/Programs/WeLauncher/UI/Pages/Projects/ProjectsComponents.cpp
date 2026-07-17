#include "UI/Pages/Projects/ProjectsComponents.h"

#include "UI/Pages/Projects/ProjectsCommands.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"

#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Declarative/UI.h"
#include "KindUI/Widgets/VirtualList.h"
#include "UI/Pages/Projects/ProjectsResources.h"
#include "UI/Pages/Projects/ProjectsStyles.h"
#include "UI/Controls/LauncherControls.h"
#include "UI/Pages/Library/ManagerViews.h"
#include "UI/Pages/Skeleton/SkeletonViews.h"

namespace we::programs::welauncher::projects {
namespace UI = we::runtime::kindui::UI;
using namespace we::runtime::kindui;

we::runtime::kindui::Element ActionToolbar(const ProjectsViewModel& vm) {
    std::vector<Element> actions;
    actions.push_back(UI::Spacer());

    if (const ProjectsCommands* commands = vm.Commands()) {
        actions.push_back(UI::Style(
            UI::Host([commands]() {
                auto btn = MakeSecondaryAction(
                    Resources::OpenProjectLabel, Icons::OpenFolderName);
                btn->SetOnClicked([cmd = commands->BrowseProject] {
                    if (cmd && cmd->CanExecute({})) {
                        cmd->Execute({});
                    }
                });
                return btn;
            }),
            Styles::Toolbar));

        actions.push_back(UI::Style(
            UI::Host([commands]() {
                auto btn = MakePrimaryAction(
                    Resources::NewProjectLabel, Icons::PlusName);
                btn->SetOnClicked([cmd = commands->NewProject] {
                    if (cmd && cmd->CanExecute({})) {
                        cmd->Execute({});
                    }
                });
                return btn;
            }),
            Styles::Toolbar));
    }

    return UI::Style(UI::Row(std::move(actions)), Styles::Toolbar);
}

we::runtime::kindui::Element SearchToolbar(const ProjectsViewModel& vm) {
    const std::string query = vm.SearchText.Get();
    ProjectsViewModel* mutableVm = const_cast<ProjectsViewModel*>(&vm);

    return UI::Style(
        UI::Row({
            UI::Spacer(),
            UI::Host([mutableVm, query]() {
                auto search = std::make_shared<CompactSearchField>(Resources::SearchPlaceholder);
                search->SetText(query);
                search->SetOnChanged([mutableVm](const std::string& text) {
                    mutableVm->SetSearchText(text);
                });
                return search;
            }),
        }),
        Styles::SearchRow);
}

we::runtime::kindui::Element ProjectTableHeader(const ProjectsViewModel& vm) {
    const ProjectSortMode sortMode = vm.SortMode.Get();
    ProjectsViewModel* mutableVm = const_cast<ProjectsViewModel*>(&vm);

    return UI::Id(
        UI::Style(
            UI::Host([sortMode, mutableVm]() {
                auto header = std::make_shared<we::programs::welauncher::ProjectTableHeader>();
                header->SetSortMode(sortMode);
                header->SetOnSort([mutableVm](ProjectSortMode mode) {
                    mutableVm->SetSortMode(mode);
                });
                return header;
            }),
            Styles::TableHeader),
        "projects-table-header");
}

we::runtime::kindui::Element ProjectTableBody(const ProjectsViewModel& vm) {
    ProjectsViewModel* mutableVm = const_cast<ProjectsViewModel*>(&vm);

    return UI::Id(
        UI::Style(
            UI::Host([mutableVm]() {
                auto list = MakeVirtualList();
                list->SetItemHeight(ProjectTableRow::kRowH);
                list->BindItems(
                    mutableVm->VisibleProjects,
                    std::function<std::shared_ptr<Widget>(const ProjectItemViewState&, size_t)>(
                        [mutableVm](const ProjectItemViewState& item, size_t) {
                            auto row = std::make_shared<ProjectTableRow>(
                                item.summary, item.selected, item.favorite);
                            row->SetOnSelect([mutableVm, index = item.sourceIndex](bool additive) {
                                mutableVm->SelectProject(index, additive);
                            });
                            row->SetOnAction([mutableVm, index = item.sourceIndex](ProjectCardAction action) {
                                mutableVm->HandleRowAction(index, action);
                            });
                            return row;
                        }));
                return list;
            }),
            Styles::TableBody),
        "projects-virtual-list");
}

we::runtime::kindui::Element ProjectsEmptyState(const ProjectsViewModel& vm) {
    const ProjectsEmptyKind kind = vm.EmptyKind.Get();
    const ProjectsCommands* commands = vm.Commands();

    if (kind == ProjectsEmptyKind::NoProjects) {
        return UI::Style(
            UI::Fill(UI::Host([]() {
                auto empty = std::make_shared<EmptyStatePanel>(
                    Resources::EmptyNoProjectsTitle,
                    Resources::EmptyNoProjectsSubtitle,
                    Icons::Cube3DName);
                return empty;
            })),
            Styles::EmptyState);
    }

    return UI::Style(
        UI::Fill(UI::Host([commands]() {
            auto empty = std::make_shared<EmptyStatePanel>(
                Resources::EmptyNoMatchesTitle,
                Resources::EmptyNoMatchesSubtitle,
                Icons::SearchName);
            if (commands && commands->ClearSearch) {
                empty->SetPrimaryAction(
                    Resources::ClearSearchLabel,
                    Icons::RefreshName,
                    [cmd = commands->ClearSearch] {
                        if (cmd && cmd->CanExecute({})) {
                            cmd->Execute({});
                        }
                    });
            }
            return empty;
        })),
        Styles::EmptyState);
}

we::runtime::kindui::Element LoadingSkeleton(int rowCount) {
    std::vector<Element> rows;
    rows.reserve(static_cast<size_t>(rowCount) + 1);
    for (int i = 0; i < rowCount; ++i) {
        rows.push_back(UI::Host([] {
            return std::make_shared<SkeletonCard>(SkeletonKind::ListRow);
        }));
    }
    rows.push_back(UI::Spacer());
    return UI::Style(UI::Column(std::move(rows)), Styles::TableBody);
}

} // namespace we::programs::welauncher::projects
