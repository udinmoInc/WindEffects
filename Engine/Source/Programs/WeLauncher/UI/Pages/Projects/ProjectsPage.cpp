#include "UI/Pages/Projects/ProjectsPage.h"

#include "UI/Pages/Projects/ProjectsView.h"

#include "KindUI/App/ViewHost.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Widgets/VirtualList.h"
#include "UI/LauncherHelpers.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

namespace we::programs::welauncher {

ProjectsPage::ProjectsPage(LauncherContext& context, IProjectsHostActions& host)
    : m_Model(context)
    , m_ViewModel(m_Model, host)
    , m_Commands(m_Model, m_ViewModel, host) {
    m_ViewModel.SetCommands(m_Commands);
    m_Model.Reload();
    m_ViewModel.RecomputeVisibleProjects();
}

void ProjectsPage::Attach(
    PageState& state,
    const std::shared_ptr<we::runtime::kindui::IWidgetContext>& context) {
    if (!context) {
        return;
    }

    if (!state.viewHost) {
        state.viewHost = std::make_unique<we::runtime::kindui::ViewHost>(context);
    }

    if (!m_Bound) {
        m_ViewModel.BindViewHost(*state.viewHost);
        state.viewHost->Refresh();
        m_Bound = true;
    } else {
        state.viewHost->Refresh();
    }

    state.root = state.viewHost->GetRoot();
    if (state.root) {
        state.root->SetHorizontalAlignment(we::runtime::kindui::HorizontalAlignment::Fill);
        state.root->SetVerticalAlignment(we::runtime::kindui::VerticalAlignment::Fill);
        if (auto box = std::dynamic_pointer_cast<we::runtime::kindui::Column>(state.root)) {
            box->Background(LColor(we::runtime::kindui::ColorToken::PanelContentBackground));
        }
    }
    state.scroll = nullptr;
}

void ProjectsPage::CaptureScroll(PageState& state) const {
    if (auto list = FindVirtualList(state.root)) {
        state.scrollOffset = list->GetScrollOffset();
    }
}

void ProjectsPage::RestoreScroll(PageState& state) const {
    if (auto list = FindVirtualList(state.root)) {
        list->SetScrollOffset(state.scrollOffset);
    }
}

void ProjectsPage::Reload() {
    m_ViewModel.ReloadFromModel();
}

void ProjectsPage::SetLoading(bool loading) {
    m_ViewModel.SetLoading(loading);
}

void ProjectsPage::SetSearchText(std::string text) {
    m_ViewModel.SetSearchText(std::move(text));
}

std::string ProjectsPage::SearchText() const {
    return m_ViewModel.SearchText.Get();
}

void ProjectsPage::SelectProject(std::size_t sourceIndex, bool additive) {
    m_ViewModel.SelectProject(sourceIndex, additive);
}

void ProjectsPage::SetSortMode(ProjectSortMode mode) {
    m_ViewModel.SetSortMode(mode);
}

void ProjectsPage::ToggleCompatibleFilter() {
    m_ViewModel.SetCompatibleOnly(!m_ViewModel.CompatibleOnly.Get());
}

std::shared_ptr<we::runtime::kindui::VirtualList> ProjectsPage::FindVirtualList(
    const std::shared_ptr<we::runtime::kindui::Widget>& root) {
    if (!root) {
        return nullptr;
    }
    if (root->GetId() == "projects-virtual-list") {
        return std::dynamic_pointer_cast<we::runtime::kindui::VirtualList>(root);
    }
    for (const auto& child : root->GetChildren()) {
        if (auto found = FindVirtualList(child)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace we::programs::welauncher
