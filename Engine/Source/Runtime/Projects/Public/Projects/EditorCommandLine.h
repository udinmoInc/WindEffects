// ==============================================================================
// WindEffects — Projects — EditorCommandLine
// Public API surface for the Projects module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
