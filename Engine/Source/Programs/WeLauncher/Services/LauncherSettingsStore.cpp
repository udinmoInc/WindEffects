#include "Services/LauncherSettingsStore.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"

#include <algorithm>

namespace we::programs::welauncher {
namespace {

void LoadBool(const nlohmann::json& root, const char* key, bool& out) {
    if (root.contains(key) && root[key].is_boolean()) {
        out = root[key].get<bool>();
    }
}

void LoadInt(const nlohmann::json& root, const char* key, int& out) {
    if (root.contains(key) && root[key].is_number_integer()) {
        out = root[key].get<int>();
    } else if (root.contains(key) && root[key].is_number()) {
        out = static_cast<int>(root[key].get<double>());
    }
}

void LoadString(const nlohmann::json& root, const char* key, std::string& out) {
    if (root.contains(key) && root[key].is_string()) {
        out = root[key].get<std::string>();
    }
}

void TrimRecent(std::vector<std::string>& recent, int limit) {
    if (limit <= 0) {
        return;
    }
    if (static_cast<int>(recent.size()) > limit) {
        recent.resize(static_cast<std::size_t>(limit));
    }
}

} // namespace

void LauncherSettingsStore::Load() {
    const auto path = PathUtils::GetLauncherSettingsPath();
    const auto json = JsonFile::TryLoad(path);
    const std::string fallbackRoot = PathUtils::ToUtf8(PathUtils::GetDefaultProjectsRoot());

    auto sanitizeRoot = [&](std::string candidate) -> std::string {
        if (candidate.empty()) {
            return fallbackRoot;
        }
        const auto wide = PathUtils::FromUtf8(candidate);
        if (wide.empty()) {
            return fallbackRoot;
        }
        std::error_code ec;
        const auto parent = wide.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                return fallbackRoot;
            }
        }
        return candidate;
    };

    if (!json) {
        m_Settings = LauncherSettings{};
        m_Settings.defaultProjectsRoot = fallbackRoot;
        m_RecentProjects.clear();
        return;
    }

    const auto& root = *json;
    m_Settings = LauncherSettings{};
    m_Settings.defaultProjectsRoot = sanitizeRoot(root.value("defaultProjectsRoot", fallbackRoot));
    m_Settings.selectedEngineRoot = root.value("selectedEngineRoot", std::string{});
    LoadString(root, "engineInstallDirectory", m_Settings.engineInstallDirectory);
    m_Settings.lastBuildConfig = root.value("lastBuildConfig", std::string{ "Development" });
    LoadString(root, "defaultTemplateId", m_Settings.defaultTemplateId);
    LoadInt(root, "recentProjectsLimit", m_Settings.recentProjectsLimit);
    if (m_Settings.recentProjectsLimit < 0) {
        m_Settings.recentProjectsLimit = 0;
    }
    LoadBool(root, "openLastProjectOnStart", m_Settings.openLastProjectOnStart);

    if (root.contains("recentProjects") && root["recentProjects"].is_array()) {
        m_RecentProjects = root["recentProjects"].get<std::vector<std::string>>();
    } else {
        m_RecentProjects.clear();
    }
    TrimRecent(m_RecentProjects, m_Settings.recentProjectsLimit);
}

void LauncherSettingsStore::Save() {
    nlohmann::json root;
    root["defaultProjectsRoot"] = m_Settings.defaultProjectsRoot;
    root["selectedEngineRoot"] = m_Settings.selectedEngineRoot;
    root["engineInstallDirectory"] = m_Settings.engineInstallDirectory;
    root["lastBuildConfig"] = m_Settings.lastBuildConfig;
    root["defaultTemplateId"] = m_Settings.defaultTemplateId;
    root["recentProjectsLimit"] = m_Settings.recentProjectsLimit;
    root["openLastProjectOnStart"] = m_Settings.openLastProjectOnStart;
    TrimRecent(m_RecentProjects, m_Settings.recentProjectsLimit);
    root["recentProjects"] = m_RecentProjects;
    (void)JsonFile::Save(PathUtils::GetLauncherSettingsPath(), root);
}

void LauncherSettingsStore::TouchRecent(const std::string& weprojPath) {
    m_RecentProjects.erase(
        std::remove(m_RecentProjects.begin(), m_RecentProjects.end(), weprojPath),
        m_RecentProjects.end());
    m_RecentProjects.insert(m_RecentProjects.begin(), weprojPath);
    TrimRecent(m_RecentProjects, m_Settings.recentProjectsLimit);
}

void LauncherSettingsStore::RemoveRecent(const std::string& weprojPath) {
    m_RecentProjects.erase(
        std::remove(m_RecentProjects.begin(), m_RecentProjects.end(), weprojPath),
        m_RecentProjects.end());
}

void LauncherSettingsStore::ResetToDefaults() {
    const std::string projectsRoot = PathUtils::ToUtf8(PathUtils::GetDefaultProjectsRoot());
    m_Settings = LauncherSettings{};
    m_Settings.defaultProjectsRoot = projectsRoot;
}

} // namespace we::programs::welauncher
