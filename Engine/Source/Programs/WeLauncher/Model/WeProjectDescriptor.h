#pragma once

#include <filesystem>
#include <nlohmann/json.h>
#include <string>
#include <vector>

namespace we::programs::welauncher {

struct WeProjectDescriptor {
    int schemaVersion = 1;
    std::string projectName;
    std::string displayName;
    std::string templateId;
    std::string engineVersion;
    std::string engineRoot;
    std::string createdUtc;
    std::string lastOpenedUtc;
    std::vector<std::string> modules;
};

inline void to_json(nlohmann::json& j, const WeProjectDescriptor& d) {
    j = nlohmann::json{
        { "schemaVersion", d.schemaVersion },
        { "projectName", d.projectName },
        { "displayName", d.displayName },
        { "templateId", d.templateId },
        { "engineVersion", d.engineVersion },
        { "engineRoot", d.engineRoot },
        { "createdUtc", d.createdUtc },
        { "lastOpenedUtc", d.lastOpenedUtc },
        { "modules", d.modules },
    };
}

inline void from_json(const nlohmann::json& j, WeProjectDescriptor& d) {
    d.schemaVersion = j.value("schemaVersion", 1);
    d.projectName = j.value("projectName", std::string{});
    d.displayName = j.value("displayName", d.projectName);
    d.templateId = j.value("templateId", "Blank");
    d.engineVersion = j.value("engineVersion", std::string{});
    d.engineRoot = j.value("engineRoot", std::string{});
    d.createdUtc = j.value("createdUtc", std::string{});
    d.lastOpenedUtc = j.value("lastOpenedUtc", std::string{});
    d.modules = j.value("modules", std::vector<std::string>{});
}

struct ProjectSummary {
    std::string weprojPath;
    std::string projectRoot;
    WeProjectDescriptor descriptor;
    bool compatible = true;
    std::string compatibilityMessage;
};

struct EngineInstallInfo {
    std::string engineRoot;
    std::string engineVersion;
    std::string displayLabel;
    bool isCurrent = false;
};

struct ProjectTemplateInfo {
    std::string id;
    std::string displayName;
    std::string description;
    std::string category;
    std::string recommendedUse;
    std::vector<std::string> platforms;
    std::vector<std::string> tags;
    std::vector<std::string> features;
    std::vector<std::string> plugins;
    std::vector<std::string> starterAssets;
    std::filesystem::path templateRoot;
};

struct LauncherSettings {
    std::string defaultProjectsRoot;
    std::string selectedEngineRoot;
    std::string lastBuildConfig = "Development";
    std::string defaultTemplateId = "Blank";
    std::string theme = "Graphite Dark";
    std::string language = "English";
    std::string accentColor = "#5B8DEF";
    std::string iconStyle = "Outline";
    std::string rhiBackend = "Vulkan";
    std::string loggingLevel = "Info";
    float uiScale = 1.0f;
    float fontSize = 13.0f;
    bool autoSave = true;
    bool checkUpdatesOnStartup = true;
    bool developerMode = false;
    bool gpuValidation = false;
    bool renderGraphDebug = false;
    bool ecsDebug = false;
    bool crashDumps = true;
    bool showExperimental = false;
    bool telemetry = false;
    bool downloadOverMetered = false;
};

} // namespace we::programs::welauncher
