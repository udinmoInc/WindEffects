#pragma once

#include "App/LauncherContext.h"
#include "Model/WeProjectDescriptor.h"

#include <string>
#include <vector>

namespace we::programs::welauncher {

class ProjectsModel {
public:
    explicit ProjectsModel(LauncherContext& context);

    [[nodiscard]] const std::vector<ProjectSummary>& AllProjects() const { return m_Projects; }

    void Reload();
    [[nodiscard]] bool IsFavorite(const std::string& weprojPath) const;
    void ToggleFavorite(const std::string& weprojPath);

    [[nodiscard]] bool OpenProject(const std::string& weprojPath, std::string& statusOut);
    [[nodiscard]] bool CloneSelected(const ProjectSummary& project, std::string& statusOut);
    [[nodiscard]] bool DeleteProject(const ProjectSummary& project, std::string& statusOut);
    [[nodiscard]] bool ShowInExplorer(const ProjectSummary& project, std::string& statusOut);
    void RegenerateFiles(const ProjectSummary& project, std::string& statusOut);

private:
    LauncherContext& m_Context;
    std::vector<ProjectSummary> m_Projects;
    std::vector<std::string> m_FavoritePaths;
};

} // namespace we::programs::welauncher
