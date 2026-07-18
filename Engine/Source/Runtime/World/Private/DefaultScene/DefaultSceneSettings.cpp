#include "DefaultScene/DefaultSceneSettings.h"

#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace we::runtime::world {

namespace {

constexpr const char* kSectionName = "Editor.DefaultScene";

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseBool(const std::string& value, bool fallback) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
    return fallback;
}

float ParseFloat(const std::string& value, float fallback) {
    try {
        return std::stof(Trim(value));
    } catch (...) {
        return fallback;
    }
}

int ParseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(Trim(value));
    } catch (...) {
        return fallback;
    }
}

std::unordered_map<std::string, std::string> BuildDefaultsMap(const DefaultSceneSettings& defaults) {
    return {
        { "EnableDefaultScene", defaults.enableDefaultScene ? "true" : "false" },
        { "CreateDirectionalLight", defaults.createDirectionalLight ? "true" : "false" },
        { "CreateSkyLight", defaults.createSkyLight ? "true" : "false" },
        { "CreateSkyAtmosphere", defaults.createSkyAtmosphere ? "true" : "false" },
        { "CreateFog", defaults.createFog ? "true" : "false" },
        { "CreateGroundPlane", defaults.createGroundPlane ? "true" : "false" },
        { "SunIntensity", std::to_string(defaults.sunIntensity) },
        { "SunTemperature", std::to_string(defaults.sunTemperature) },
        { "SunRotationPitch", std::to_string(defaults.sunRotationPitch) },
        { "SunRotationYaw", std::to_string(defaults.sunRotationYaw) },
        { "SkyIntensity", std::to_string(defaults.skyIntensity) },
        { "SkyRealTimeCapture", defaults.skyRealTimeCapture ? "true" : "false" },
        { "FogDensity", std::to_string(defaults.fogDensity) },
    };
}

} // namespace

DefaultSceneSettingsLoader& DefaultSceneSettingsLoader::Get() {
    static DefaultSceneSettingsLoader instance;
    return instance;
}

const DefaultSceneSettings& DefaultSceneSettingsLoader::GetSettings() {
    EnsureLoaded();
    return m_Settings;
}

std::filesystem::path DefaultSceneSettingsLoader::GetConfigPath() const {
    return we::core::ResolveEditorConfigPath("function.ini");
}

void DefaultSceneSettingsLoader::EnsureLoaded() {
    if (m_Loaded) {
        return;
    }

    const auto path = GetConfigPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    const DefaultSceneSettings defaults{};
    EnsureConfigHasDefaults(path, defaults);
    m_Settings = ParseSettings(path, defaults);
    m_Loaded = true;
}

void DefaultSceneSettingsLoader::EnsureConfigHasDefaults(const std::filesystem::path& path, const DefaultSceneSettings& defaults) const {
    const auto defaultValues = BuildDefaultsMap(defaults);

    std::vector<std::string> lines;
    {
        std::ifstream input(path);
        std::string line;
        while (std::getline(input, line)) {
            lines.push_back(line);
        }
    }

    bool sectionFound = false;
    std::unordered_map<std::string, bool> present;
    std::string currentSection;
    for (const std::string& rawLine : lines) {
        const std::string line = Trim(rawLine);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            currentSection = Trim(line.substr(1, line.size() - 2));
            if (currentSection == kSectionName) {
                sectionFound = true;
            }
            continue;
        }

        if (currentSection != kSectionName) {
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        present[Trim(line.substr(0, equals))] = true;
    }

    std::vector<std::string> missingLines;
    for (const auto& [key, value] : defaultValues) {
        if (!present.contains(key)) {
            missingLines.push_back(key + "=" + value);
        }
    }

    if (sectionFound && missingLines.empty()) {
        return;
    }

    std::ofstream output(path, std::ios::app);
    if (!output.is_open()) {
        return;
    }

    if (!sectionFound) {
        if (!lines.empty()) {
            output << "\n";
        }
        output << "[" << kSectionName << "]\n";
    }
    for (const std::string& line : missingLines) {
        output << line << "\n";
    }
}

DefaultSceneSettings DefaultSceneSettingsLoader::ParseSettings(
    const std::filesystem::path& path,
    const DefaultSceneSettings& defaults) const {

    DefaultSceneSettings result = defaults;
    std::ifstream input(path);
    if (!input.is_open()) {
        return result;
    }

    std::unordered_map<std::string, std::string> entries;
    std::string currentSection;
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            currentSection = Trim(line.substr(1, line.size() - 2));
            continue;
        }
        if (currentSection != kSectionName) {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, equals));
        const std::string value = Trim(line.substr(equals + 1));
        entries[key] = value;
    }

    const auto get = [&](const char* key) -> std::string {
        auto it = entries.find(key);
        return it != entries.end() ? it->second : "";
    };

    result.enableDefaultScene = ParseBool(get("EnableDefaultScene"), result.enableDefaultScene);
    result.createDirectionalLight = ParseBool(get("CreateDirectionalLight"), result.createDirectionalLight);
    result.createSkyLight = ParseBool(get("CreateSkyLight"), result.createSkyLight);
    result.createSkyAtmosphere = ParseBool(get("CreateSkyAtmosphere"), result.createSkyAtmosphere);
    result.createFog = ParseBool(get("CreateFog"), result.createFog);
    result.createGroundPlane = ParseBool(get("CreateGroundPlane"), result.createGroundPlane);

    result.sunIntensity = std::max(0.0f, ParseFloat(get("SunIntensity"), result.sunIntensity));
    result.sunTemperature = std::max(1000, ParseInt(get("SunTemperature"), result.sunTemperature));
    result.sunRotationPitch = ParseFloat(get("SunRotationPitch"), result.sunRotationPitch);
    result.sunRotationYaw = ParseFloat(get("SunRotationYaw"), result.sunRotationYaw);
    result.skyIntensity = std::max(0.0f, ParseFloat(get("SkyIntensity"), result.skyIntensity));
    result.skyRealTimeCapture = ParseBool(get("SkyRealTimeCapture"), result.skyRealTimeCapture);
    result.fogDensity = std::max(0.0f, ParseFloat(get("FogDensity"), result.fogDensity));

    return result;
}

} // namespace we::runtime::world
