#include "UI/Pages/Projects/ProjectsView.h"

#include "KindUI/Declarative/UI.h"
#include "UI/Shell/LauncherHelpers.h"
#include "UI/Pages/Projects/ProjectsComponents.h"
#include "UI/Pages/Projects/ProjectsResources.h"
#include "UI/Pages/Projects/ProjectsStyles.h"
#include "UI/Controls/LauncherControls.h"

namespace we::programs::welauncher {
namespace UI = we::runtime::kindui::UI;

we::runtime::kindui::Element BuildProjectsView(const ProjectsViewModel& viewModel) {
    const float s = LScale();

    std::vector<we::runtime::kindui::Element> chromeChildren;
    chromeChildren.push_back(
        UI::Style(
            UI::Row({
                UI::Section(projects::Resources::PageTitle, projects::Resources::PageSubtitle),
                projects::ActionToolbar(viewModel),
            }),
            projects::Styles::Chrome));

    chromeChildren.push_back(
        UI::Host([s]() { return std::make_shared<FixedGap>(1.0f, kLauncherTitleToToolbar * s); }));
    chromeChildren.push_back(projects::SearchToolbar(viewModel));
    chromeChildren.push_back(
        UI::Host([s]() { return std::make_shared<FixedGap>(1.0f, kLauncherToolbarToDivider * s); }));

    auto chrome = UI::Padding(
        UI::Column(std::move(chromeChildren)),
        kLauncherContentPadX * s,
        kLauncherContentPadTop * s,
        kLauncherContentPadX * s,
        0.0f);

    std::vector<we::runtime::kindui::Element> tableChildren;
    tableChildren.push_back(projects::ProjectTableHeader(viewModel));

    if (viewModel.Loading.Get()) {
        tableChildren.push_back(projects::LoadingSkeleton(8));
    } else if (viewModel.EmptyKind.Get() != ProjectsEmptyKind::None) {
        tableChildren.push_back(projects::ProjectsEmptyState(viewModel));
    } else {
        tableChildren.push_back(UI::Fill(projects::ProjectTableBody(viewModel)));
    }

    return UI::Style(
        UI::Column({
            chrome,
            UI::Fill(UI::Style(UI::Column(std::move(tableChildren)), projects::Styles::TableHost)),
        }),
        projects::Styles::PageRoot);
}

} // namespace we::programs::welauncher
