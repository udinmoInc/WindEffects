// ==============================================================================
// WindEffects — WeLauncher — IProjectsHostActions
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "UI/Pages/Projects/ProjectsTypes.h"

#include <cstddef>
#include <string>

namespace we::programs::welauncher {

/// Shell-provided actions for modals, navigation, and status (not owned by Projects Model).
class IProjectsHostActions {
public:
    virtual ~IProjectsHostActions() = default;

    virtual void ShowCreateWizard() = 0;
    virtual void BrowseForProject() = 0;
    virtual void ShowRenameDialog() = 0;
    virtual void CommitRenameProject(const std::string& newName) = 0;
    virtual void ShowProjectMoreMenu(std::size_t sourceIndex) = 0;
    virtual void SetStatus(const std::string& message) = 0;
    virtual void UpdateFooter() = 0;
};

} // namespace we::programs::welauncher
