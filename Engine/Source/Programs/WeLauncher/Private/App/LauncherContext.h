#pragma once

#include "Services/EditorLaunchService.h"
#include "Services/EngineDiscoveryService.h"
#include "Services/LauncherSettingsStore.h"
#include "Services/ProjectService.h"
#include "Services/ProjectTemplateRegistry.h"
#include "Services/SdkValidationService.h"

#include <memory>
#include <string>

namespace we::programs::welauncher {

class LauncherContext {
public:
    LauncherContext();

    bool Initialize();
    void Save();

    [[nodiscard]] EngineDiscoveryService& Engines() { return m_Engines; }
    [[nodiscard]] ProjectTemplateRegistry& Templates() { return m_Templates; }
    [[nodiscard]] LauncherSettingsStore& Settings() { return m_Settings; }
    [[nodiscard]] ProjectService& Projects() { return m_Projects; }
    [[nodiscard]] EditorLaunchService& EditorLaunch() { return m_EditorLaunch; }
    [[nodiscard]] SdkValidationService& Sdk() { return m_Sdk; }

    void SetStatusMessage(std::string message) { m_StatusMessage = std::move(message); }
    [[nodiscard]] const std::string& StatusMessage() const { return m_StatusMessage; }

private:
    EngineDiscoveryService m_Engines;
    ProjectTemplateRegistry m_Templates;
    LauncherSettingsStore m_Settings;
    ProjectService m_Projects;
    EditorLaunchService m_EditorLaunch;
    SdkValidationService m_Sdk;
    std::string m_StatusMessage;
};

} // namespace we::programs::welauncher
