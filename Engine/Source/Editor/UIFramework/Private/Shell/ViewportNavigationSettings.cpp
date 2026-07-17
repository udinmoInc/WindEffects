#include "WindEffects/Editor/UI/Shell/ViewportNavigationSettings.h"

#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::editor::viewport {

namespace {

std::filesystem::path GetConfigPath() {
    return we::core::ResolveEditorConfigPath("editor_viewport.ini");
}

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool ParseBool(const std::string& value, bool fallback) {
    const std::string lower = Trim(value);
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

} // namespace

ViewportNavigationSettingsStore& ViewportNavigationSettingsStore::Get() {
    static ViewportNavigationSettingsStore instance;
    return instance;
}

std::string ViewportNavigationSettingsStore::PresetToString(NavigationPreset preset) {
    switch (preset) {
    case NavigationPreset::UE5: return "UE5";
    case NavigationPreset::Blender: return "Blender";
    case NavigationPreset::Maya: return "Maya";
    case NavigationPreset::Unity: return "Unity";
    case NavigationPreset::Custom: return "Custom";
    }
    return "UE5";
}

NavigationPreset ViewportNavigationSettingsStore::PresetFromString(const std::string& value) {
    const std::string normalized = Trim(value);
    if (normalized == "Blender") return NavigationPreset::Blender;
    if (normalized == "Maya") return NavigationPreset::Maya;
    if (normalized == "Unity") return NavigationPreset::Unity;
    if (normalized == "Custom") return NavigationPreset::Custom;
    return NavigationPreset::UE5;
}

void ViewportNavigationSettingsStore::ApplyPreset(NavigationPreset preset) {
    m_Settings.preset = preset;
    switch (preset) {
    case NavigationPreset::UE5:
        m_Settings.mouseSensitivity = 0.15f;
        m_Settings.cameraAcceleration = 1.0f;
        m_Settings.cameraSmoothing = 12.0f;
        m_Settings.invertX = false;
        m_Settings.invertY = false;
        m_Settings.defaultCameraSpeed = 4.0f;
        m_Settings.maxBoostMultiplier = 4.0f;
        m_Settings.slowMultiplier = 0.25f;
        m_Settings.orbitAroundSelection = true;
        m_Settings.focusOnSelection = true;
        m_Settings.scrollWheelSpeedMultiplier = 1.0f;
        break;
    case NavigationPreset::Blender:
        m_Settings.mouseSensitivity = 0.2f;
        m_Settings.cameraAcceleration = 1.0f;
        m_Settings.cameraSmoothing = 10.0f;
        m_Settings.invertX = false;
        m_Settings.invertY = false;
        m_Settings.defaultCameraSpeed = 5.0f;
        m_Settings.maxBoostMultiplier = 3.0f;
        m_Settings.slowMultiplier = 0.2f;
        m_Settings.orbitAroundSelection = true;
        m_Settings.focusOnSelection = true;
        m_Settings.scrollWheelSpeedMultiplier = 1.0f;
        break;
    case NavigationPreset::Maya:
        m_Settings.mouseSensitivity = 0.12f;
        m_Settings.cameraAcceleration = 1.0f;
        m_Settings.cameraSmoothing = 14.0f;
        m_Settings.invertX = false;
        m_Settings.invertY = false;
        m_Settings.defaultCameraSpeed = 4.0f;
        m_Settings.maxBoostMultiplier = 4.0f;
        m_Settings.slowMultiplier = 0.25f;
        m_Settings.orbitAroundSelection = true;
        m_Settings.focusOnSelection = true;
        m_Settings.scrollWheelSpeedMultiplier = 1.0f;
        break;
    case NavigationPreset::Unity:
        m_Settings.mouseSensitivity = 0.18f;
        m_Settings.cameraAcceleration = 1.0f;
        m_Settings.cameraSmoothing = 12.0f;
        m_Settings.invertX = false;
        m_Settings.invertY = false;
        m_Settings.defaultCameraSpeed = 4.0f;
        m_Settings.maxBoostMultiplier = 4.0f;
        m_Settings.slowMultiplier = 0.25f;
        m_Settings.orbitAroundSelection = true;
        m_Settings.focusOnSelection = true;
        m_Settings.scrollWheelSpeedMultiplier = 1.0f;
        break;
    case NavigationPreset::Custom:
        break;
    }
}

void ViewportNavigationSettingsStore::EnsureLoaded() {
    if (m_Loaded) {
        return;
    }
    Load();
    m_Loaded = true;
}

void ViewportNavigationSettingsStore::Load() {
    const auto path = GetConfigPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ifstream file(path);
    if (!file.is_open()) {
        ApplyPreset(NavigationPreset::UE5);
        Save();
        return;
    }

    ViewportNavigationSettings defaults{};
    ApplyPreset(NavigationPreset::UE5);
    defaults = m_Settings;

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const std::string key = Trim(line.substr(0, equals));
        const std::string value = Trim(line.substr(equals + 1));

        if (key == "NavigationPreset") m_Settings.preset = PresetFromString(value);
        else if (key == "MouseSensitivity") m_Settings.mouseSensitivity = std::max(0.01f, ParseFloat(value, defaults.mouseSensitivity));
        else if (key == "CameraAcceleration") m_Settings.cameraAcceleration = std::max(0.1f, ParseFloat(value, defaults.cameraAcceleration));
        else if (key == "CameraSmoothing") m_Settings.cameraSmoothing = std::max(0.0f, ParseFloat(value, defaults.cameraSmoothing));
        else if (key == "InvertX") m_Settings.invertX = ParseBool(value, defaults.invertX);
        else if (key == "InvertY") m_Settings.invertY = ParseBool(value, defaults.invertY);
        else if (key == "DefaultCameraSpeed") m_Settings.defaultCameraSpeed = ParseFloat(value, defaults.defaultCameraSpeed);
        else if (key == "MaxBoostMultiplier") m_Settings.maxBoostMultiplier = std::max(1.0f, ParseFloat(value, defaults.maxBoostMultiplier));
        else if (key == "SlowMultiplier") m_Settings.slowMultiplier = std::clamp(ParseFloat(value, defaults.slowMultiplier), 0.01f, 1.0f);
        else if (key == "OrbitAroundSelection") m_Settings.orbitAroundSelection = ParseBool(value, defaults.orbitAroundSelection);
        else if (key == "FocusOnSelection") m_Settings.focusOnSelection = ParseBool(value, defaults.focusOnSelection);
        else if (key == "ScrollWheelSpeedMultiplier") m_Settings.scrollWheelSpeedMultiplier = std::max(0.1f, ParseFloat(value, defaults.scrollWheelSpeedMultiplier));
        else if (key == "CameraSpeed") m_Settings.defaultCameraSpeed = ParseFloat(value, defaults.defaultCameraSpeed);
    }
}

void ViewportNavigationSettingsStore::Save() const {
    const auto path = GetConfigPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }

    file << "# WindEffects editor viewport navigation settings\n";
    file << "[ViewportNavigation]\n";
    file << "NavigationPreset=" << PresetToString(m_Settings.preset) << "\n";
    file << "MouseSensitivity=" << m_Settings.mouseSensitivity << "\n";
    file << "CameraAcceleration=" << m_Settings.cameraAcceleration << "\n";
    file << "CameraSmoothing=" << m_Settings.cameraSmoothing << "\n";
    file << "InvertX=" << (m_Settings.invertX ? "true" : "false") << "\n";
    file << "InvertY=" << (m_Settings.invertY ? "true" : "false") << "\n";
    file << "DefaultCameraSpeed=" << m_Settings.defaultCameraSpeed << "\n";
    file << "CameraSpeed=" << m_Settings.defaultCameraSpeed << "\n";
    file << "MaxBoostMultiplier=" << m_Settings.maxBoostMultiplier << "\n";
    file << "SlowMultiplier=" << m_Settings.slowMultiplier << "\n";
    file << "OrbitAroundSelection=" << (m_Settings.orbitAroundSelection ? "true" : "false") << "\n";
    file << "FocusOnSelection=" << (m_Settings.focusOnSelection ? "true" : "false") << "\n";
    file << "ScrollWheelSpeedMultiplier=" << m_Settings.scrollWheelSpeedMultiplier << "\n";
}

} // namespace we::editor::viewport
