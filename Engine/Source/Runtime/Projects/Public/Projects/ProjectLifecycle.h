#pragma once

#include "Projects/Export.h"
#include "Projects/WeProjectDescriptor.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace we::projects {

struct PROJECTS_API ProjectCreateRequest {
    std::string displayName;
    std::string templateId = "Blank";
    std::filesystem::path parentDirectory;
    std::string engineVersion;
    std::string engineRoot;
};

struct PROJECTS_API ProjectOperationResult {
    bool success = false;
    std::string message;
    std::filesystem::path weprojPath;
    std::filesystem::path projectRoot;
};

class PROJECTS_API ProjectLifecycle {
public:
    [[nodiscard]] static std::optional<WeProjectDescriptor> ReadDescriptor(
        const std::filesystem::path& weprojPath);

    [[nodiscard]] static bool WriteDescriptor(
        const std::filesystem::path& weprojPath,
        const WeProjectDescriptor& descriptor);

    [[nodiscard]] static ProjectValidationResult Validate(
        const WeProjectDescriptor& descriptor,
        const std::string& installedEngineVersion);

    [[nodiscard]] static ProjectValidationResult ValidateProjectPath(
        const std::filesystem::path& weprojPath,
        const std::string& installedEngineVersion);

    [[nodiscard]] static ProjectOperationResult CreateNewProject(const ProjectCreateRequest& request);

    [[nodiscard]] static ProjectOperationResult CloneProject(
        const std::filesystem::path& sourceWeproj,
        const std::string& newDisplayName,
        const std::filesystem::path& parentDirectory);

    [[nodiscard]] static ProjectOperationResult UpgradeProject(
        const std::filesystem::path& weprojPath,
        const std::string& targetEngineVersion,
        const std::string& targetEngineRoot);

    [[nodiscard]] static bool EnsureProjectLayout(
        const std::filesystem::path& projectRoot,
        const std::string& projectName);

    [[nodiscard]] static std::string SanitizeProjectName(const std::string& displayName);
    [[nodiscard]] static std::string NowUtc();
};

} // namespace we::projects
