#include "App/LauncherContext.h"

#include "Util/PathUtils.h"
#include "Core/Logger.h"

namespace we::programs::welauncher {

LauncherContext::LauncherContext()
    : m_Projects(m_Engines, m_Templates, m_Settings)
    , m_EditorLaunch(m_Engines, m_Settings, m_Projects) {
}

bool LauncherContext::Initialize() {
    HE_INFO("[WeLauncher] Settings.Load...");
    m_Settings.Load();
    HE_INFO("[WeLauncher] Engines.Discover...");
    if (!m_Engines.Discover(PathUtils::GetExecutableDirectory())) {
        m_StatusMessage = "Warning: engine installation not found near launcher.";
        HE_WARN("[WeLauncher] Engine discovery failed");
    } else if (m_Settings.Settings().selectedEngineRoot.empty()) {
        m_Settings.Settings().selectedEngineRoot = PathUtils::ToUtf8(m_Engines.Current().engineRoot);
        HE_INFO("[WeLauncher] Engine discovered: " + m_Engines.Current().engineVersion);
    }
    HE_INFO("[WeLauncher] Templates.Load...");
    m_Templates.Load(m_Engines.Current());
    HE_INFO("[WeLauncher] Templates loaded: " + std::to_string(m_Templates.Templates().size()));
    return true;
}

void LauncherContext::Save() {
    m_Settings.Save();
}

} // namespace we::programs::welauncher
