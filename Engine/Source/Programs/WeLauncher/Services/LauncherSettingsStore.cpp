#include "Services/LauncherSettingsStore.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"

#include <algorithm>

namespace we::programs::welauncher {

void LauncherSettingsStore::Load() {
    const auto path = PathUtils::GetLauncherSettingsPath();
    const auto json = JsonFile::TryLoad(path);
    if (!json) {
        m_Settings.defaultProjectsRoot = PathUtils::ToUtf8(PathUtils::GetDefaultProjectsRoot());
        return;
    }

    const auto& root = *json;
    m_Settings.defaultProjectsRoot = root.value(
        "defaultProjectsRoot", PathUtils::ToUtf8(PathUtils::GetDefaultProjectsRoot()));
    m_Settings.selectedEngineRoot = root.value("selectedEngineRoot", std::string{});
    m_Settings.lastBuildConfig = root.value("lastBuildConfig", "Debug");
    m_RecentProjects = root.value("recentProjects", std::vector<std::string>{});
}

void LauncherSettingsStore::Save() {
    nlohmann::json root;
    root["defaultProjectsRoot"] = m_Settings.defaultProjectsRoot;
    root["selectedEngineRoot"] = m_Settings.selectedEngineRoot;
    root["lastBuildConfig"] = m_Settings.lastBuildConfig;
    root["recentProjects"] = m_RecentProjects;
    (void)JsonFile::Save(PathUtils::GetLauncherSettingsPath(), root);
}

void LauncherSettingsStore::TouchRecent(const std::string& weprojPath) {
    m_RecentProjects.erase(
        std::remove(m_RecentProjects.begin(), m_RecentProjects.end(), weprojPath),
        m_RecentProjects.end());
    m_RecentProjects.insert(m_RecentProjects.begin(), weprojPath);
    constexpr std::size_t kMaxRecent = 32;
    if (m_RecentProjects.size() > kMaxRecent) {
        m_RecentProjects.resize(kMaxRecent);
    }
}

void LauncherSettingsStore::RemoveRecent(const std::string& weprojPath) {
    m_RecentProjects.erase(
        std::remove(m_RecentProjects.begin(), m_RecentProjects.end(), weprojPath),
        m_RecentProjects.end());
}

} // namespace we::programs::welauncher
