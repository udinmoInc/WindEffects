#pragma once

#include "App/LauncherContext.h"
#include "UI/PageState.h"
#include "UI/Pages/Projects/IProjectsHostActions.h"
#include "UI/Pages/Projects/ProjectsCommands.h"
#include "UI/Pages/Projects/ProjectsModel.h"
#include "UI/Pages/Projects/ProjectsViewModel.h"
#include "UI/Widgets/LauncherControls.h"

#include <memory>

namespace we::runtime::kindui {
class IWidgetContext;
class VirtualList;
class Widget;
}

namespace we::programs::welauncher {

class ProjectsPage {
public:
    explicit ProjectsPage(LauncherContext& context, IProjectsHostActions& host);

    void Attach(PageState& state, const std::shared_ptr<we::runtime::kindui::IWidgetContext>& context);
    void CaptureScroll(PageState& state) const;
    void RestoreScroll(PageState& state) const;

    void Reload();
    void SetLoading(bool loading);
    void SetSearchText(std::string text);
    [[nodiscard]] std::string SearchText() const;

    void SelectProject(std::size_t sourceIndex, bool additive = false);
    void HandleProjectAction(std::size_t sourceIndex, ProjectCardAction action);
    void SetSortMode(ProjectSortMode mode);
    void ToggleCompatibleFilter();

    [[nodiscard]] ProjectsViewModel& ViewModel() { return m_ViewModel; }
    [[nodiscard]] const ProjectsViewModel& ViewModel() const { return m_ViewModel; }
    [[nodiscard]] ProjectsModel& Model() { return m_Model; }
    [[nodiscard]] int SelectedIndex() const { return m_ViewModel.SelectedIndex.Get(); }
    [[nodiscard]] const std::vector<std::size_t>& MultiSelected() const {
        return m_ViewModel.MultiSelected();
    }
    [[nodiscard]] const std::vector<ProjectSummary>& AllProjects() const {
        return m_Model.AllProjects();
    }
    [[nodiscard]] ProjectSortMode SortMode() const { return m_ViewModel.SortMode.Get(); }

private:
    static std::shared_ptr<we::runtime::kindui::VirtualList> FindVirtualList(
        const std::shared_ptr<we::runtime::kindui::Widget>& root);

    ProjectsModel m_Model;
    ProjectsViewModel m_ViewModel;
    ProjectsCommands m_Commands;
    bool m_Bound = false;
};

} // namespace we::programs::welauncher
