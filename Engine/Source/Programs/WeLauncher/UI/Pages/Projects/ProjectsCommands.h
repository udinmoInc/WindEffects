#pragma once

#include "KindUI/Commands/ICommandRegistry.h"
#include "UI/Pages/Projects/IProjectsHostActions.h"
#include "UI/Pages/Projects/ProjectsModel.h"
#include "UI/Pages/Projects/ProjectsTypes.h"

#include <memory>

namespace we::programs::welauncher {

class ProjectsViewModel;

/// Reusable command objects for toolbar, context menu, and keyboard shortcuts.
class ProjectsCommands {
public:
    ProjectsCommands(ProjectsModel& model, ProjectsViewModel& viewModel, IProjectsHostActions& host);

    [[nodiscard]] std::shared_ptr<we::runtime::kindui::ICommand> Get(ProjectCardAction action) const;

    std::shared_ptr<we::runtime::kindui::ICommand> NewProject;
    std::shared_ptr<we::runtime::kindui::ICommand> BrowseProject;
    std::shared_ptr<we::runtime::kindui::ICommand> OpenProject;
    std::shared_ptr<we::runtime::kindui::ICommand> DeleteProject;
    std::shared_ptr<we::runtime::kindui::ICommand> RenameProject;
    std::shared_ptr<we::runtime::kindui::ICommand> CloneProject;
    std::shared_ptr<we::runtime::kindui::ICommand> RefreshProjects;
    std::shared_ptr<we::runtime::kindui::ICommand> ShowInExplorer;
    std::shared_ptr<we::runtime::kindui::ICommand> ClearSearch;
    std::shared_ptr<we::runtime::kindui::ICommand> ToggleFavorite;
    std::shared_ptr<we::runtime::kindui::ICommand> ShowMoreMenu;
    std::shared_ptr<we::runtime::kindui::ICommand> RegenerateFiles;

private:
    ProjectsModel& m_Model;
    ProjectsViewModel& m_ViewModel;
    IProjectsHostActions& m_Host;
};

} // namespace we::programs::welauncher
