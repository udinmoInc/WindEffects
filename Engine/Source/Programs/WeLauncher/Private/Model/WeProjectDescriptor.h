#pragma once

#include "Projects/WeProjectDescriptor.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace we::programs::welauncher {

/// Canonical .weproj schema — single type shared with Runtime/Projects.
using WeProjectDescriptor = we::projects::WeProjectDescriptor;

struct ProjectSummary {
    std::string weprojPath;
    std::string projectRoot;
    WeProjectDescriptor descriptor;
    bool compatible = true;
    std::string compatibilityMessage;
    // Derived display fields for the project manager table / details panel.
    std::string statusLabel = "Unknown";
    std::string projectType = "Project";
    std::string platforms = "Windows";
    std::string description;
    std::uint64_t diskBytes = 0;
};

struct EngineInstallInfo {
    std::string engineRoot;
    std::string engineVersion;
    std::string displayLabel;
    std::string buildId = "Development";
    std::string sdkStatus = "Unknown";
    int pluginCount = 0;
    std::string updateStatus = "Up to date";
    bool isCurrent = true;
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
    std::string engineInstallDirectory;
    std::string lastBuildConfig = "Development";
    std::string defaultTemplateId = "Blank";
    std::string qualityPreset = "Balanced"; // wizard quality dropdown
    int recentProjectsLimit = 20; // 0 = unlimited
    bool openLastProjectOnStart = false;
};

inline constexpr const char* kLauncherVersion = "1.0.0";

} // namespace we::programs::welauncher
