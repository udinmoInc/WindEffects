#include "Services/LauncherSettingsStore.h"

#include "Util/JsonFile.h"
#include "Util/PathUtils.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <algorithm>
#include <exception>
#include <string>

namespace we::programs::welauncher {
namespace {

void LoadBool(const nlohmann::json& root, const char* key, bool& out) {
    if (root.contains(key) && root[key].is_boolean()) {
        out = root[key].get<bool>();
    }
}

void LoadInt(const nlohmann::json& root, const char* key, int& out) {
    if (!root.contains(key)) {
        return;
    }
    const auto& value = root.at(key);
    if (value.is_number_integer()) {
        out = value.get<int>();
    } else if (value.is_number()) {
        out = static_cast<int>(value.get<double>());
    }
}

void LoadString(const nlohmann::json& root, const char* key, std::string& out) {
    if (root.contains(key) && root.at(key).is_string()) {
        out = root.at(key).get<std::string>();
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

void LoadRecentProjects(const nlohmann::json& root, std::vector<std::string>& out) {
    out.clear();
    if (!root.contains("recentProjects") || !root.at("recentProjects").is_array()) {
        return;
    }
    for (const auto& entry : root.at("recentProjects")) {
        if (!entry.is_string()) {
            continue;
        }
        std::string path = entry.get<std::string>();
        if (!path.empty()) {
            out.push_back(std::move(path));
        }
    }
}

} // namespace

void LauncherSettingsStore::Load() {
    m_RecentProjects.clear();
    m_Settings = LauncherSettings{};

    try {
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

        if (!json || !json->is_object()) {
            m_Settings.defaultProjectsRoot = fallbackRoot;
            return;
        }

        const auto& root = *json;
        LoadString(root, "defaultProjectsRoot", m_Settings.defaultProjectsRoot);
        m_Settings.defaultProjectsRoot = sanitizeRoot(m_Settings.defaultProjectsRoot);
        LoadString(root, "selectedEngineRoot", m_Settings.selectedEngineRoot);
        LoadString(root, "engineInstallDirectory", m_Settings.engineInstallDirectory);
        LoadString(root, "lastBuildConfig", m_Settings.lastBuildConfig);
        if (m_Settings.lastBuildConfig.empty()) {
            m_Settings.lastBuildConfig = "Development";
        }
        LoadString(root, "defaultTemplateId", m_Settings.defaultTemplateId);
        if (m_Settings.defaultTemplateId.empty()) {
            m_Settings.defaultTemplateId = "Blank";
        }
        LoadString(root, "qualityPreset", m_Settings.qualityPreset);
        if (m_Settings.qualityPreset.empty()) {
            m_Settings.qualityPreset = "Balanced";
        }
        LoadInt(root, "recentProjectsLimit", m_Settings.recentProjectsLimit);
        if (m_Settings.recentProjectsLimit < 0) {
            m_Settings.recentProjectsLimit = 0;
        }
        LoadBool(root, "openLastProjectOnStart", m_Settings.openLastProjectOnStart);
        LoadRecentProjects(root, m_RecentProjects);
        TrimRecent(m_RecentProjects, m_Settings.recentProjectsLimit);
    } catch (...) {
        const std::string fallbackRoot = PathUtils::ToUtf8(PathUtils::GetDefaultProjectsRoot());
        m_Settings = LauncherSettings{};
        m_Settings.defaultProjectsRoot = fallbackRoot;
        m_RecentProjects.clear();
    }
}

void LauncherSettingsStore::Save() {
    try {
        nlohmann::json root = nlohmann::json::object();
        root["defaultProjectsRoot"] = m_Settings.defaultProjectsRoot;
        root["selectedEngineRoot"] = m_Settings.selectedEngineRoot;
        root["engineInstallDirectory"] = m_Settings.engineInstallDirectory;
        root["lastBuildConfig"] = m_Settings.lastBuildConfig;
        root["defaultTemplateId"] = m_Settings.defaultTemplateId;
        root["qualityPreset"] = m_Settings.qualityPreset;
        root["recentProjectsLimit"] = m_Settings.recentProjectsLimit;
        root["openLastProjectOnStart"] = m_Settings.openLastProjectOnStart;
        TrimRecent(m_RecentProjects, m_Settings.recentProjectsLimit);
        root["recentProjects"] = m_RecentProjects;
        (void)JsonFile::Save(PathUtils::GetLauncherSettingsPath(), root);
    } catch (const std::exception& ex) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            std::string("Failed to persist launcher settings: ") + ex.what());
    } catch (...) {
        WE_LOG_WARN(we::LogCategory::General.data(),
            "Failed to persist launcher settings (unknown error).");
    }
}

void LauncherSettingsStore::TouchRecent(const std::string& weprojPath) {
    if (weprojPath.empty()) {
        return;
    }
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
    m_RecentProjects.clear();
}

} // namespace we::programs::welauncher
