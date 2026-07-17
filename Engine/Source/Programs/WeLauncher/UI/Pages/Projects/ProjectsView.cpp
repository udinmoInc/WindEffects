#include "UI/Pages/Projects/ProjectsView.h"

#include "KindUI/Declarative/UI.h"
#include "UI/Shell/LauncherHelpers.h"
#include "UI/Pages/Projects/ProjectsComponents.h"
#include "UI/Pages/Projects/ProjectsStyles.h"

namespace we::programs::welauncher {
namespace UI = we::runtime::kindui::UI;

we::runtime::kindui::Element BuildProjectsView(const ProjectsViewModel& viewModel) {
    const float s = LScale();

    // Semantic page column — one container per section, no mixed title/toolbar rows.
    //
    // ProjectsPage
    // ├── Header
    // ├── Toolbar (Open / New | spacer | Search)
    // ├── Divider
    // └── Content (table header + list | empty | skeleton)
    //
    // Status bar lives on the shell, not this page.
    return UI::Style(
        UI::Padding(
            UI::Column({
                projects::PageHeader(),
                UI::Margin(projects::PageToolbar(viewModel), 0.0f, kLauncherTitleToToolbar * s, 0.0f, 0.0f),
                UI::Margin(projects::PageDivider(), 0.0f, kLauncherToolbarToDivider * s, 0.0f, 0.0f),
                UI::Margin(projects::PageContent(viewModel), 0.0f, kLauncherDividerToTable * s, 0.0f, 0.0f),
            }),
            kLauncherContentPadX * s,
            kLauncherContentPadTop * s,
            kLauncherContentPadX * s,
            kLauncherContentPadBottom * s),
        projects::Styles::PageRoot);
}

} // namespace we::programs::welauncher
