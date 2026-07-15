#include "Environment/EnvironmentSettings.h"

#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace we::runtime::world::environment {

namespace {

constexpr const char* kSectionName = "Editor.Environment";

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

} // namespace

EnvironmentSettingsLoader& EnvironmentSettingsLoader::Get() {
    static EnvironmentSettingsLoader instance;
    return instance;
}

const EnvironmentSettings& EnvironmentSettingsLoader::GetSettings() {
    EnsureLoaded();
    return m_Settings;
}

void EnvironmentSettingsLoader::SaveSettings(const EnvironmentSettings& settings) {
    m_Settings = settings;
    m_Loaded = true;

    const auto path = GetConfigPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::unordered_map<std::string, std::string> entries;
    std::string currentSection;
    std::vector<std::string> lines;
    {
        std::ifstream input(path);
        std::string line;
        while (std::getline(input, line)) {
            const std::string trimmed = Trim(line);
            if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
                currentSection = Trim(trimmed.substr(1, trimmed.size() - 2));
            }
            if (currentSection != kSectionName) {
                lines.push_back(line);
            }
        }
    }

    std::ofstream output(path, std::ios::trunc);
    for (const std::string& line : lines) {
        output << line << "\n";
    }
    if (!lines.empty() && !lines.back().empty()) {
        output << "\n";
    }

    output << "[" << kSectionName << "]\n";
    output << "AutoCreateOnNewLevel=" << (settings.autoCreateOnNewLevel ? "true" : "false") << "\n";
    output << "CreateDirectionalLight=" << (settings.createDirectionalLight ? "true" : "false") << "\n";
    output << "CreateSkyLight=" << (settings.createSkyLight ? "true" : "false") << "\n";
    output << "CreateSkyAtmosphere=" << (settings.createSkyAtmosphere ? "true" : "false") << "\n";
    output << "CreateHeightFog=" << (settings.createHeightFog ? "true" : "false") << "\n";
    output << "CreateVolumetricClouds=" << (settings.createVolumetricClouds ? "true" : "false") << "\n";
    output << "EnableVolumetricFog=" << (settings.enableVolumetricFog ? "true" : "false") << "\n";
    output << "SunIntensity=" << settings.sunIntensity << "\n";
    output << "SunTemperature=" << settings.sunTemperature << "\n";
    output << "SunRotationPitch=" << settings.sunRotationPitch << "\n";
    output << "SunRotationYaw=" << settings.sunRotationYaw << "\n";
    output << "SkyLightIntensity=" << settings.skyLightIntensity << "\n";
    output << "SkyLightRealTimeCapture=" << (settings.skyLightRealTimeCapture ? "true" : "false") << "\n";
    output << "FogDensity=" << settings.fogDensity << "\n";
    output << "FogHeightFalloff=" << settings.fogHeightFalloff << "\n";
    output << "CloudCoverage=" << settings.cloudCoverage << "\n";
    output << "CloudAltitude=" << settings.cloudAltitude << "\n";
}

std::filesystem::path EnvironmentSettingsLoader::GetConfigPath() const {
    return we::core::ResolveEditorConfigPath("function.ini");
}

void EnvironmentSettingsLoader::EnsureLoaded() {
    if (m_Loaded) {
        return;
    }

    const auto path = GetConfigPath();
    EnvironmentSettings defaults{};
    m_Settings = defaults;

    std::ifstream input(path);
    if (!input.is_open()) {
        m_Loaded = true;
        return;
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
        if (currentSection != kSectionName && currentSection != "Editor.DefaultScene") {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        entries[Trim(line.substr(0, equals))] = Trim(line.substr(equals + 1));
    }

    const auto get = [&](const char* key) -> std::string {
        auto it = entries.find(key);
        return it != entries.end() ? it->second : "";
    };

    m_Settings.autoCreateOnNewLevel = ParseBool(get("AutoCreateOnNewLevel"), ParseBool(get("EnableDefaultScene"), m_Settings.autoCreateOnNewLevel));
    m_Settings.createDirectionalLight = ParseBool(get("CreateDirectionalLight"), m_Settings.createDirectionalLight);
    m_Settings.createSkyLight = ParseBool(get("CreateSkyLight"), m_Settings.createSkyLight);
    m_Settings.createSkyAtmosphere = ParseBool(get("CreateSkyAtmosphere"), m_Settings.createSkyAtmosphere);
    m_Settings.createHeightFog = ParseBool(get("CreateFog"), ParseBool(get("CreateHeightFog"), m_Settings.createHeightFog));
    m_Settings.createVolumetricClouds = ParseBool(get("CreateVolumetricClouds"), m_Settings.createVolumetricClouds);
    m_Settings.enableVolumetricFog = ParseBool(get("EnableVolumetricFog"), m_Settings.enableVolumetricFog);
    m_Settings.sunIntensity = std::max(0.0f, ParseFloat(get("SunIntensity"), m_Settings.sunIntensity));
    m_Settings.sunTemperature = std::max(1000, ParseInt(get("SunTemperature"), m_Settings.sunTemperature));
    m_Settings.sunRotationPitch = ParseFloat(get("SunRotationPitch"), m_Settings.sunRotationPitch);
    m_Settings.sunRotationYaw = ParseFloat(get("SunRotationYaw"), m_Settings.sunRotationYaw);
    m_Settings.skyLightIntensity = std::max(0.0f, ParseFloat(get("SkyIntensity"), ParseFloat(get("SkyLightIntensity"), m_Settings.skyLightIntensity)));
    m_Settings.skyLightRealTimeCapture = ParseBool(get("SkyRealTimeCapture"), ParseBool(get("SkyLightRealTimeCapture"), m_Settings.skyLightRealTimeCapture));
    m_Settings.fogDensity = std::max(0.0f, ParseFloat(get("FogDensity"), m_Settings.fogDensity));
    m_Settings.fogHeightFalloff = std::max(0.0f, ParseFloat(get("FogHeightFalloff"), m_Settings.fogHeightFalloff));
    m_Settings.cloudCoverage = std::clamp(ParseFloat(get("CloudCoverage"), m_Settings.cloudCoverage), 0.0f, 1.0f);
    m_Settings.cloudAltitude = std::max(50.0f, ParseFloat(get("CloudAltitude"), m_Settings.cloudAltitude));

    m_Loaded = true;
}

} // namespace we::runtime::world::environment
