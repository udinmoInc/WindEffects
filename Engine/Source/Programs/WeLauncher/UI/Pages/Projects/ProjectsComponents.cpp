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
#include "UI/Shell/LauncherHelpers.h"

namespace we::programs::welauncher::projects {
namespace UI = we::runtime::kindui::UI;
using namespace we::runtime::kindui;

namespace {

Element LeftActions(const ProjectsViewModel& vm) {
    std::vector<Element> actions;
    if (const ProjectsCommands* commands = vm.Commands()) {
        actions.push_back(UI::AlignStart(UI::Style(
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
            Styles::Toolbar)));

        actions.push_back(UI::AlignStart(UI::Style(
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
            Styles::Toolbar)));
    }
    return UI::Gap(
        UI::AlignStart(UI::Row(std::move(actions))),
        kLauncherButtonGap * LScale());
}

Element SearchField(const ProjectsViewModel& vm) {
    const std::string query = vm.SearchText.Get();
    ProjectsViewModel* mutableVm = const_cast<ProjectsViewModel*>(&vm);
    const float s = LScale();

    return UI::AlignEnd(UI::Width(
        UI::Style(
            UI::Host([mutableVm, query]() {
                auto search = std::make_shared<CompactSearchField>(Resources::SearchPlaceholder);
                search->SetText(query);
                search->SetOnChanged([mutableVm](const std::string& text) {
                    mutableVm->SetSearchText(text);
                });
                return search;
            }),
            Styles::SearchRow),
        kLauncherSearchW * s));
}

} // namespace

Element PageHeader() {
    return UI::Style(
        UI::AlignStart(UI::Section(Resources::PageTitle, Resources::PageSubtitle)),
        Styles::Header);
}

Element PageToolbar(const ProjectsViewModel& vm) {
    // Single horizontal toolbar: left actions | flexible spacer | right-aligned search.
    // Title never lives here.
    return UI::Style(
        UI::Gap(
            UI::Row({
                LeftActions(vm),
                UI::Spacer(),
                SearchField(vm),
            }),
            kLauncherToolbarControlGap * LScale()),
        Styles::Toolbar);
}

Element PageDivider() {
    return UI::Style(
        UI::Host([]() { return std::make_shared<ThinDivider>(); }),
        Styles::Divider);
}

Element ProjectTableHeader(const ProjectsViewModel& vm) {
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

Element ProjectTableBody(const ProjectsViewModel& vm) {
    ProjectsViewModel* mutableVm = const_cast<ProjectsViewModel*>(&vm);

    return UI::Id(
        UI::Style(
            UI::Host([mutableVm]() {
                auto list = MakeVirtualList();
                list->SetItemHeight(ProjectTableRow::kRowH);
                list->SetFlexGrow(1.0f);
                list->SetFlexBasis(0.0f);
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

Element ProjectsEmptyState(const ProjectsViewModel& vm) {
    const ProjectsEmptyKind kind = vm.EmptyKind.Get();
    const ProjectsCommands* commands = vm.Commands();

    if (kind == ProjectsEmptyKind::NoProjects) {
        return UI::Style(
            UI::Fill(UI::Host([]() {
                return std::make_shared<EmptyStatePanel>(
                    Resources::EmptyNoProjectsTitle,
                    Resources::EmptyNoProjectsSubtitle,
                    Icons::Cube3DName);
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

Element PageContent(const ProjectsViewModel& vm) {
    // Content owns list / empty only — never toolbar or title.
    std::vector<Element> children;
    children.push_back(ProjectTableHeader(vm));

    if (vm.EmptyKind.Get() != ProjectsEmptyKind::None) {
        children.push_back(ProjectsEmptyState(vm));
    } else {
        children.push_back(UI::Fill(ProjectTableBody(vm)));
    }

    return UI::Fill(UI::Style(
        UI::Column(std::move(children)),
        Styles::Content));
}

} // namespace we::programs::welauncher::projects
