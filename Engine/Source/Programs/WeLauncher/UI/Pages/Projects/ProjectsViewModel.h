#pragma once

#include "KindUI/Core/Observable.h"
#include "Model/WeProjectDescriptor.h"
#include "UI/Controls/LauncherControls.h"
#include "UI/Pages/Projects/ProjectsTypes.h"

#include <cstddef>
#include <string>
#include <vector>

namespace we::runtime::kindui {
class ViewHost;
}

namespace we::programs::welauncher {

class ProjectsCommands;
class ProjectsModel;
class IProjectsHostActions;

enum class ProjectsEmptyKind {
    None,
    NoProjects,
    NoMatches,
};

struct ProjectItemViewState {
    ProjectSummary summary;
    std::size_t sourceIndex = 0;
    bool selected = false;
    bool favorite = false;
};

/// Observable presentation state for the Projects page.
class ProjectsViewModel {
public:
    ProjectsViewModel(ProjectsModel& model, IProjectsHostActions& host);

    void SetCommands(ProjectsCommands& commands);
    void BindViewHost(we::runtime::kindui::ViewHost& host);

    void ReloadFromModel();
    void RecomputeVisibleProjects();

    void SetSearchText(std::string text);
    void SetSortMode(ProjectSortMode mode);
    void SetCompatibleOnly(bool enabled);
    void SetLoading(bool loading);

    void SelectProject(std::size_t sourceIndex, bool additive);
    void HandleRowAction(std::size_t sourceIndex, ProjectCardAction action);

    [[nodiscard]] std::size_t SelectedSourceIndex() const;
    [[nodiscard]] const ProjectSummary* SelectedProject() const;
    [[nodiscard]] const std::vector<std::size_t>& MultiSelected() const { return m_MultiSelected; }
    [[nodiscard]] ProjectsCommands* Commands() const { return m_Commands; }

    we::runtime::kindui::Observable<std::string> SearchText;
    we::runtime::kindui::Observable<ProjectSortMode> SortMode;
    we::runtime::kindui::Observable<bool> CompatibleOnly;
    we::runtime::kindui::Observable<bool> Loading;
    we::runtime::kindui::Observable<int> SelectedIndex;
    we::runtime::kindui::Observable<ProjectsEmptyKind> EmptyKind;
    we::runtime::kindui::ObservableList<ProjectItemViewState> VisibleProjects;

private:
    [[nodiscard]] std::size_t MapSourceIndex(const ProjectSummary& project, std::size_t fallback) const;
    [[nodiscard]] bool IsSelected(std::size_t sourceIndex) const;

    ProjectsModel& m_Model;
    IProjectsHostActions& m_Host;
    ProjectsCommands* m_Commands = nullptr;
    std::vector<std::size_t> m_MultiSelected;
};

} // namespace we::programs::welauncher
