#include "UI/Pages/Projects/ProjectsCommands.h"

#include "UI/Pages/Projects/ProjectsViewModel.h"

#include "KindUI/Commands/LambdaCommand.h"

namespace we::programs::welauncher {

ProjectsCommands::ProjectsCommands(
    ProjectsModel& model,
    ProjectsViewModel& viewModel,
    IProjectsHostActions& host)
    : m_Model(model)
    , m_ViewModel(viewModel)
    , m_Host(host) {
    using we::runtime::kindui::MakeCommand;

    NewProject = MakeCommand("projects.new", "New Project", [this](const auto&) {
        m_Host.ShowCreateWizard();
    });

    BrowseProject = MakeCommand("projects.browse", "Open Project", [this](const auto&) {
        m_Host.BrowseForProject();
    });

    OpenProject = MakeCommand(
        "projects.open",
        "Open",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                m_Host.SetStatus("Select a project first");
                return;
            }
            std::string status;
            (void)m_Model.OpenProject(selected->weprojPath, status);
            m_Host.SetStatus(status);
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    DeleteProject = MakeCommand(
        "projects.delete",
        "Delete",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                return;
            }
            std::string status;
            if (m_Model.DeleteProject(*selected, status)) {
                m_ViewModel.ReloadFromModel();
                m_Host.UpdateFooter();
            }
            m_Host.SetStatus(status);
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    RenameProject = MakeCommand(
        "projects.rename",
        "Rename",
        [this](const auto&) { m_Host.ShowRenameDialog(); },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    CloneProject = MakeCommand(
        "projects.clone",
        "Duplicate",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                return;
            }
            std::string status;
            if (m_Model.CloneSelected(*selected, status)) {
                m_ViewModel.ReloadFromModel();
                m_Host.UpdateFooter();
            }
            m_Host.SetStatus(status);
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    RefreshProjects = MakeCommand("projects.refresh", "Refresh", [this](const auto&) {
        m_ViewModel.ReloadFromModel();
        m_Host.UpdateFooter();
        m_Host.SetStatus("Project list refreshed");
    });

    ShowInExplorer = MakeCommand(
        "projects.show-in-explorer",
        "Show In Explorer",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                return;
            }
            std::string status;
            (void)m_Model.ShowInExplorer(*selected, status);
            m_Host.SetStatus(status);
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    ClearSearch = MakeCommand("projects.clear-search", "Clear Search", [this](const auto&) {
        m_ViewModel.SetSearchText({});
    });

    ToggleFavorite = MakeCommand(
        "projects.favorite",
        "Favorite",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                return;
            }
            m_Model.ToggleFavorite(selected->weprojPath);
            m_ViewModel.RecomputeVisibleProjects();
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    ShowMoreMenu = MakeCommand(
        "projects.more",
        "More",
        [this](const auto&) {
            m_Host.ShowProjectMoreMenu(m_ViewModel.SelectedSourceIndex());
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });

    RegenerateFiles = MakeCommand(
        "projects.regenerate",
        "Regenerate",
        [this](const auto&) {
            const ProjectSummary* selected = m_ViewModel.SelectedProject();
            if (!selected) {
                return;
            }
            std::string status;
            m_Model.RegenerateFiles(*selected, status);
            m_Host.SetStatus(status);
        },
        [this](const auto&) { return m_ViewModel.SelectedProject() != nullptr; });
}

std::shared_ptr<we::runtime::kindui::ICommand> ProjectsCommands::Get(ProjectCardAction action) const {
    switch (action) {
    case ProjectCardAction::Open: return OpenProject;
    case ProjectCardAction::Clone: return CloneProject;
    case ProjectCardAction::Rename: return RenameProject;
    case ProjectCardAction::Delete: return DeleteProject;
    case ProjectCardAction::ShowInExplorer: return ShowInExplorer;
    case ProjectCardAction::Favorite: return ToggleFavorite;
    case ProjectCardAction::More: return ShowMoreMenu;
    case ProjectCardAction::Regenerate: return RegenerateFiles;
    default: return nullptr;
    }
}

} // namespace we::programs::welauncher
