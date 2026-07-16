#pragma once

#include "Model/WeProjectDescriptor.h"
#include "Services/EngineDiscoveryService.h"
#include "Services/LauncherSettingsStore.h"
#include "Services/ProjectTemplateRegistry.h"

#include <optional>
#include <string>

namespace we::programs::welauncher {

struct ProjectOperationResult {
    bool success = false;
    std::string message;
    std::string weprojPath;
};

class ProjectService {
public:
    ProjectService(
        EngineDiscoveryService& engines,
        ProjectTemplateRegistry& templates,
        LauncherSettingsStore& settings);

    [[nodiscard]] std::optional<ProjectSummary> LoadSummary(const std::filesystem::path& weprojPath) const;
    [[nodiscard]] std::vector<ProjectSummary> LoadRecentSummaries() const;

    ProjectOperationResult CreateProject(
        const std::string& displayName,
        const std::string& templateId,
        const std::filesystem::path& parentDirectory);

    ProjectOperationResult OpenProject(const std::filesystem::path& weprojPath);
    ProjectOperationResult CloneProject(const std::filesystem::path& sourceWeproj, const std::string& newDisplayName);
    ProjectOperationResult RenameProject(const std::filesystem::path& weprojPath, const std::string& newDisplayName);
    ProjectOperationResult DeleteProject(const std::filesystem::path& weprojPath);
    ProjectOperationResult RepairProject(const std::filesystem::path& weprojPath);

    [[nodiscard]] bool ValidateCompatibility(const WeProjectDescriptor& descriptor, std::string& outMessage) const;

private:
    bool WriteDescriptor(const std::filesystem::path& weprojPath, const WeProjectDescriptor& descriptor) const;
    bool EnsureProjectLayout(const std::filesystem::path& projectRoot, const std::string& projectName) const;
    bool MaterializeTemplate(
        const ProjectTemplateInfo& tmpl,
        const std::filesystem::path& projectRoot,
        const std::string& projectName) const;
    void CopyTemplateTree(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        const std::string& projectName) const;

    EngineDiscoveryService& m_Engines;
    ProjectTemplateRegistry& m_Templates;
    LauncherSettingsStore& m_Settings;
};

} // namespace we::programs::welauncher
