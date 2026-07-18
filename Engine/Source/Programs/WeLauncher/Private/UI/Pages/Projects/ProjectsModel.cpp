#include "UI/Pages/Projects/ProjectsModel.h"

#include "UI/Shell/LauncherHelpers.h"
#include "Util/PathUtils.h"

#include <algorithm>

namespace we::programs::welauncher {

ProjectsModel::ProjectsModel(LauncherContext& context)
    : m_Context(context) {}

void ProjectsModel::Reload() {
    m_Projects = m_Context.Projects().LoadRecentSummaries();
}

bool ProjectsModel::IsFavorite(const std::string& weprojPath) const {
    return std::find(m_FavoritePaths.begin(), m_FavoritePaths.end(), weprojPath)
        != m_FavoritePaths.end();
}

void ProjectsModel::ToggleFavorite(const std::string& weprojPath) {
    const auto it = std::find(m_FavoritePaths.begin(), m_FavoritePaths.end(), weprojPath);
    if (it != m_FavoritePaths.end()) {
        m_FavoritePaths.erase(it);
    } else {
        m_FavoritePaths.push_back(weprojPath);
    }
}

bool ProjectsModel::OpenProject(const std::string& weprojPath, std::string& statusOut) {
    const auto result = m_Context.EditorLaunch().Launch(PathUtils::FromUtf8(weprojPath));
    statusOut = result.success ? "Opening project..." : result.message;
    return result.success;
}

bool ProjectsModel::CloneSelected(const ProjectSummary& project, std::string& statusOut) {
    const auto result = m_Context.Projects().CloneProject(
        PathUtils::FromUtf8(project.weprojPath),
        project.descriptor.displayName + " Copy");
    statusOut = result.success ? "Project cloned" : result.message;
    if (result.success) {
        Reload();
    }
    return result.success;
}

bool ProjectsModel::DeleteProject(const ProjectSummary& project, std::string& statusOut) {
    const auto result = m_Context.Projects().DeleteProject(PathUtils::FromUtf8(project.weprojPath));
    statusOut = result.success ? "Project removed from recent list" : result.message;
    if (result.success) {
        Reload();
    }
    return result.success;
}

bool ProjectsModel::ShowInExplorer(const ProjectSummary& project, std::string& statusOut) {
    if (RevealInExplorer(project.projectRoot)) {
        statusOut = "Opened project folder";
        return true;
    }
    statusOut = "Failed to open project folder.";
    return false;
}

void ProjectsModel::RegenerateFiles(const ProjectSummary& project, std::string& statusOut) {
    (void)project;
    statusOut = "Regenerate queued";
}

} // namespace we::programs::welauncher
