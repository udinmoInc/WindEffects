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

void LoadFloat(const nlohmann::json& root, const char* key, float& out) {
    if (root.contains(key) && root[key].is_number()) {
        out = root[key].get<float>();
    }
}

void LoadString(const nlohmann::json& root, const char* key, std::string& out) {
    if (root.contains(key) && root[key].is_string()) {
        out = root[key].get<std::string>();
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
        return;
    }

    const auto& root = *json;
    m_Settings = LauncherSettings{};
    m_Settings.defaultProjectsRoot = sanitizeRoot(root.value("defaultProjectsRoot", fallbackRoot));
    m_Settings.selectedEngineRoot = root.value("selectedEngineRoot", std::string{});
    m_Settings.lastBuildConfig = root.value("lastBuildConfig", std::string{ "Development" });
    LoadString(root, "defaultTemplateId", m_Settings.defaultTemplateId);
    LoadString(root, "theme", m_Settings.theme);
    LoadString(root, "language", m_Settings.language);
    LoadString(root, "accentColor", m_Settings.accentColor);
    LoadString(root, "iconStyle", m_Settings.iconStyle);
    LoadString(root, "rhiBackend", m_Settings.rhiBackend);
    LoadString(root, "loggingLevel", m_Settings.loggingLevel);
    LoadFloat(root, "uiScale", m_Settings.uiScale);
    LoadFloat(root, "fontSize", m_Settings.fontSize);
    LoadBool(root, "autoSave", m_Settings.autoSave);
    LoadBool(root, "checkUpdatesOnStartup", m_Settings.checkUpdatesOnStartup);
    LoadBool(root, "developerMode", m_Settings.developerMode);
    LoadBool(root, "gpuValidation", m_Settings.gpuValidation);
    LoadBool(root, "renderGraphDebug", m_Settings.renderGraphDebug);
    LoadBool(root, "ecsDebug", m_Settings.ecsDebug);
    LoadBool(root, "crashDumps", m_Settings.crashDumps);
    LoadBool(root, "showExperimental", m_Settings.showExperimental);
    LoadBool(root, "telemetry", m_Settings.telemetry);
    LoadBool(root, "downloadOverMetered", m_Settings.downloadOverMetered);
    if (root.contains("recentProjects") && root["recentProjects"].is_array()) {
        m_RecentProjects = root["recentProjects"].get<std::vector<std::string>>();
    } else {
        m_RecentProjects.clear();
    }
}

void LauncherSettingsStore::Save() {
    nlohmann::json root;
    root["defaultProjectsRoot"] = m_Settings.defaultProjectsRoot;
    root["selectedEngineRoot"] = m_Settings.selectedEngineRoot;
    root["lastBuildConfig"] = m_Settings.lastBuildConfig;
    root["defaultTemplateId"] = m_Settings.defaultTemplateId;
    root["theme"] = m_Settings.theme;
    root["language"] = m_Settings.language;
    root["accentColor"] = m_Settings.accentColor;
    root["iconStyle"] = m_Settings.iconStyle;
    root["rhiBackend"] = m_Settings.rhiBackend;
    root["loggingLevel"] = m_Settings.loggingLevel;
    root["uiScale"] = m_Settings.uiScale;
    root["fontSize"] = m_Settings.fontSize;
    root["autoSave"] = m_Settings.autoSave;
    root["checkUpdatesOnStartup"] = m_Settings.checkUpdatesOnStartup;
    root["developerMode"] = m_Settings.developerMode;
    root["gpuValidation"] = m_Settings.gpuValidation;
    root["renderGraphDebug"] = m_Settings.renderGraphDebug;
    root["ecsDebug"] = m_Settings.ecsDebug;
    root["crashDumps"] = m_Settings.crashDumps;
    root["showExperimental"] = m_Settings.showExperimental;
    root["telemetry"] = m_Settings.telemetry;
    root["downloadOverMetered"] = m_Settings.downloadOverMetered;
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
