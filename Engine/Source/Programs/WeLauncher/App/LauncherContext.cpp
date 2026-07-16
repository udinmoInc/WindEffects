#include "App/LauncherContext.h"

#include "Util/PathUtils.h"

namespace we::programs::welauncher {

LauncherContext::LauncherContext()
    : m_Projects(m_Engines, m_Templates, m_Settings)
    , m_EditorLaunch(m_Engines, m_Settings, m_Projects) {
}

bool LauncherContext::Initialize() {
    m_Settings.Load();
    if (!m_Engines.Discover(PathUtils::GetExecutableDirectory())) {
        m_StatusMessage = "Warning: engine installation not found near launcher.";
    } else if (m_Settings.Settings().selectedEngineRoot.empty()) {
        m_Settings.Settings().selectedEngineRoot = PathUtils::ToUtf8(m_Engines.Current().engineRoot);
    }
    m_Templates.Load(m_Engines.Current());
    return true;
}

void LauncherContext::Save() {
    m_Settings.Save();
}

} // namespace we::programs::welauncher
