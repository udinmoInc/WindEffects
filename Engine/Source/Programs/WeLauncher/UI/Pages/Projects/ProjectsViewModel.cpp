#include "UI/Pages/Projects/ProjectsViewModel.h"

#include "KindUI/App/ViewHost.h"
#include "UI/Shell/LauncherHelpers.h"
#include "UI/Pages/Projects/ProjectsCommands.h"
#include "UI/Pages/Projects/ProjectsView.h"

#include <algorithm>

namespace we::programs::welauncher {

ProjectsViewModel::ProjectsViewModel(ProjectsModel& model, IProjectsHostActions& host)
    : m_Model(model)
    , m_Host(host)
    , SearchText("")
    , SortMode(ProjectSortMode::Recent)
    , CompatibleOnly(false)
    , SelectedIndex(-1)
    , EmptyKind(ProjectsEmptyKind::None) {}

void ProjectsViewModel::SetCommands(ProjectsCommands& commands) {
    m_Commands = &commands;
}

void ProjectsViewModel::BindViewHost(we::runtime::kindui::ViewHost& host) {
    host.SetViewFactory([this] { return BuildProjectsView(*this); });

    host.Observe(SearchText);
    host.Observe(SortMode);
    host.Observe(CompatibleOnly);
    host.Observe(SelectedIndex);
    host.Observe(EmptyKind);
    host.ObserveList(VisibleProjects);
}

void ProjectsViewModel::ReloadFromModel() {
    m_Model.Reload();
    if (SelectedIndex.Get() >= static_cast<int>(m_Model.AllProjects().size())) {
        SelectedIndex = m_Model.AllProjects().empty() ? -1 : 0;
    }
    RecomputeVisibleProjects();
}

void ProjectsViewModel::RecomputeVisibleProjects() {
    const std::string query = ToLowerCopy(SearchText.Get());
    std::vector<ProjectItemViewState> visible;
    visible.reserve(m_Model.AllProjects().size());

    for (std::size_t i = 0; i < m_Model.AllProjects().size(); ++i) {
        const auto& project = m_Model.AllProjects()[i];
        if (CompatibleOnly.Get() && !project.compatible) {
            continue;
        }
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                project.descriptor.displayName + " " + project.projectRoot + " "
                + project.descriptor.templateId + " " + project.descriptor.engineVersion);
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        ProjectItemViewState item;
        item.summary = project;
        item.sourceIndex = i;
        item.selected = IsSelected(i);
        item.favorite = m_Model.IsFavorite(project.weprojPath);
        visible.push_back(std::move(item));
    }

    switch (SortMode.Get()) {
    case ProjectSortMode::Name:
        std::stable_sort(visible.begin(), visible.end(), [](const ProjectItemViewState& a, const ProjectItemViewState& b) {
            return ToLowerCopy(a.summary.descriptor.displayName)
                < ToLowerCopy(b.summary.descriptor.displayName);
        });
        break;
    case ProjectSortMode::Engine:
        std::stable_sort(visible.begin(), visible.end(), [](const ProjectItemViewState& a, const ProjectItemViewState& b) {
            if (a.summary.descriptor.engineVersion != b.summary.descriptor.engineVersion) {
                return a.summary.descriptor.engineVersion < b.summary.descriptor.engineVersion;
            }
            return ToLowerCopy(a.summary.descriptor.displayName)
                < ToLowerCopy(b.summary.descriptor.displayName);
        });
        break;
    case ProjectSortMode::Recent:
    default:
        std::stable_sort(visible.begin(), visible.end(), [](const ProjectItemViewState& a, const ProjectItemViewState& b) {
            return a.summary.descriptor.lastOpenedUtc > b.summary.descriptor.lastOpenedUtc;
        });
        break;
    }

    if (m_Model.AllProjects().empty() && query.empty()) {
        EmptyKind = ProjectsEmptyKind::NoProjects;
    } else if (visible.empty()) {
        EmptyKind = ProjectsEmptyKind::NoMatches;
    } else {
        EmptyKind = ProjectsEmptyKind::None;
    }

    VisibleProjects.Set(std::move(visible));
}

void ProjectsViewModel::SetSearchText(std::string text) {
    SearchText = std::move(text);
    RecomputeVisibleProjects();
}

void ProjectsViewModel::SetSortMode(ProjectSortMode mode) {
    SortMode = mode;
    RecomputeVisibleProjects();
}

void ProjectsViewModel::SetCompatibleOnly(bool enabled) {
    CompatibleOnly = enabled;
    RecomputeVisibleProjects();
}

void ProjectsViewModel::SelectProject(std::size_t sourceIndex, bool additive) {
    if (m_Model.AllProjects().empty() || sourceIndex >= m_Model.AllProjects().size()) {
        SelectedIndex = -1;
        m_MultiSelected.clear();
        RecomputeVisibleProjects();
        return;
    }

    if (additive) {
        const auto it = std::find(m_MultiSelected.begin(), m_MultiSelected.end(), sourceIndex);
        if (it != m_MultiSelected.end()) {
            m_MultiSelected.erase(it);
        } else {
            m_MultiSelected.push_back(sourceIndex);
        }
        SelectedIndex = static_cast<int>(sourceIndex);
    } else {
        m_MultiSelected.clear();
        SelectedIndex = static_cast<int>(sourceIndex);
    }

    const auto& project = m_Model.AllProjects()[sourceIndex];
    m_Host.SetStatus("Selected: " + project.descriptor.displayName);
    RecomputeVisibleProjects();
}

void ProjectsViewModel::HandleRowAction(std::size_t sourceIndex, ProjectCardAction action) {
    SelectProject(sourceIndex, false);
    if (!m_Commands) {
        return;
    }
    if (auto command = m_Commands->Get(action)) {
        if (command->CanExecute({})) {
            command->Execute({});
        }
    }
}

std::size_t ProjectsViewModel::SelectedSourceIndex() const {
    const int idx = SelectedIndex.Get();
    return idx >= 0 ? static_cast<std::size_t>(idx) : 0;
}

const ProjectSummary* ProjectsViewModel::SelectedProject() const {
    const int idx = SelectedIndex.Get();
    if (idx < 0 || static_cast<std::size_t>(idx) >= m_Model.AllProjects().size()) {
        return nullptr;
    }
    return &m_Model.AllProjects()[static_cast<std::size_t>(idx)];
}

std::size_t ProjectsViewModel::MapSourceIndex(const ProjectSummary& project, std::size_t fallback) const {
    for (std::size_t j = 0; j < m_Model.AllProjects().size(); ++j) {
        if (m_Model.AllProjects()[j].weprojPath == project.weprojPath) {
            return j;
        }
    }
    return fallback;
}

bool ProjectsViewModel::IsSelected(std::size_t sourceIndex) const {
    if (static_cast<int>(sourceIndex) == SelectedIndex.Get()) {
        return true;
    }
    return std::find(m_MultiSelected.begin(), m_MultiSelected.end(), sourceIndex) != m_MultiSelected.end();
}

} // namespace we::programs::welauncher
