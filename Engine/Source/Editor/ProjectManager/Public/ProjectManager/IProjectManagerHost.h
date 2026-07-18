#pragma once

#include "ProjectManager/Export.h"

#include <filesystem>
#include <functional>
#include <string>

namespace we::editor::projectmanager {

/// Host callbacks implemented by the Editor executable.
struct PROJECTMANAGER_API IProjectManagerHost {
    virtual ~IProjectManagerHost() = default;

    /// Open an existing project and transition into the editor workspace.
    virtual void OnOpenProject(const std::filesystem::path& weprojPath) = 0;

    /// Optional status text for the project manager chrome.
    virtual void OnStatusMessage(const std::string& message) {}
};

} // namespace we::editor::projectmanager
