#pragma once

#include "Projects/Export.h"

#include <nlohmann/json.h>
#include <string>
#include <vector>

namespace we::projects {

/// Canonical .weproj schema. The Editor reads ONLY this file for project identity.
struct PROJECTS_API WeProjectDescriptor {
    int schemaVersion = 1;
    std::string projectName;
    std::string displayName;
    std::string templateId;
    std::string engineVersion;
    std::string engineRoot;
    std::string contentDirectory = "Content";
    std::string startupMap;
    std::string defaultGameMode;
    std::vector<std::string> modules;
    std::vector<std::string> plugins;
    std::vector<std::string> targetPlatforms{ "Windows" };
    nlohmann::json settings = nlohmann::json::object();
    std::string createdUtc;
    std::string lastOpenedUtc;
};

inline void to_json(nlohmann::json& j, const WeProjectDescriptor& d) {
    j = nlohmann::json{
        { "schemaVersion", d.schemaVersion },
        { "projectName", d.projectName },
        { "displayName", d.displayName },
        { "templateId", d.templateId },
        { "engineVersion", d.engineVersion },
        { "engineRoot", d.engineRoot },
        { "contentDirectory", d.contentDirectory },
        { "startupMap", d.startupMap },
        { "defaultGameMode", d.defaultGameMode },
        { "modules", d.modules },
        { "plugins", d.plugins },
        { "targetPlatforms", d.targetPlatforms },
        { "settings", d.settings },
        { "createdUtc", d.createdUtc },
        { "lastOpenedUtc", d.lastOpenedUtc },
    };
}

inline void from_json(const nlohmann::json& j, WeProjectDescriptor& d) {
    d.schemaVersion = j.value("schemaVersion", 1);
    d.projectName = j.value("projectName", std::string{});
    d.displayName = j.value("displayName", d.projectName);
    d.templateId = j.value("templateId", "Blank");
    d.engineVersion = j.value("engineVersion", std::string{});
    d.engineRoot = j.value("engineRoot", std::string{});
    d.contentDirectory = j.value("contentDirectory", std::string{ "Content" });
    d.startupMap = j.value("startupMap", std::string{});
    d.defaultGameMode = j.value("defaultGameMode", std::string{});
    d.modules = j.value("modules", std::vector<std::string>{});
    d.plugins = j.value("plugins", std::vector<std::string>{});
    d.targetPlatforms = j.value("targetPlatforms", std::vector<std::string>{ "Windows" });
    d.settings = j.value("settings", nlohmann::json::object());
    d.createdUtc = j.value("createdUtc", std::string{});
    d.lastOpenedUtc = j.value("lastOpenedUtc", std::string{});

    // Legacy ProjectSettings.json fields may be mirrored under settings.
    if (d.startupMap.empty() && d.settings.contains("startupMap")) {
        d.startupMap = d.settings.value("startupMap", std::string{});
    }
    if (d.defaultGameMode.empty() && d.settings.contains("defaultGameMode")) {
        d.defaultGameMode = d.settings.value("defaultGameMode", std::string{});
    }
}

struct PROJECTS_API ProjectValidationResult {
    bool ok = false;
    bool needsUpgrade = false;
    bool missingSdk = false;
    std::string message;
};

} // namespace we::projects
