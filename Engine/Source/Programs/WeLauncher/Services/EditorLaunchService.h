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
