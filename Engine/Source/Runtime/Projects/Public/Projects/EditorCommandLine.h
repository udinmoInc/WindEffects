#pragma once

#include "Projects/Export.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace we::projects {

struct PROJECTS_API EditorCommandLine {
    std::optional<std::filesystem::path> projectPath;
    bool forceProjectManager = false;
    bool newProject = false;
    bool safeMode = false;
    bool recoveryMode = false;
    std::vector<std::string> rawArgs;
};

[[nodiscard]] PROJECTS_API EditorCommandLine ParseEditorCommandLine(int argc, char* argv[]);

} // namespace we::projects
