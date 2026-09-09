// ==============================================================================
// WindEffects — WeLauncher — EditorLaunchService
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Model/WeProjectDescriptor.h"
#include "Services/EngineDiscoveryService.h"
#include "Services/LauncherSettingsStore.h"
#include "Services/ProjectService.h"

namespace we::programs::welauncher {

struct EditorLaunchResult {
    bool success = false;
    std::string message;
};

class EditorLaunchService {
public:
    EditorLaunchService(
        EngineDiscoveryService& engines,
        LauncherSettingsStore& settings,
        ProjectService& projects);

    EditorLaunchResult Launch(const std::filesystem::path& weprojPath);

private:
    EngineDiscoveryService& m_Engines;
    LauncherSettingsStore& m_Settings;
    ProjectService& m_Projects;
};

} // namespace we::programs::welauncher
