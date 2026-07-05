#pragma once

#include "Application/Export.hpp"

#include <string>

namespace we::programs::editor {

enum class NavigationPreset {
    UE5,
    Blender,
    Maya,
    Unity,
    Custom
};

// UE5-style viewport navigation settings, persisted across editor sessions.
struct ViewportNavigationSettings {
    NavigationPreset preset = NavigationPreset::UE5;

    float mouseSensitivity = 0.15f;
    float cameraAcceleration = 1.0f;
    float cameraSmoothing = 12.0f;
    bool invertX = false;
    bool invertY = false;

    float defaultCameraSpeed = 4.0f;
    float maxBoostMultiplier = 4.0f;
    float slowMultiplier = 0.25f;

    bool orbitAroundSelection = true;
    bool focusOnSelection = true;
    float scrollWheelSpeedMultiplier = 1.0f;
};

class APPLICATION_API ViewportNavigationSettingsStore {
public:
    static ViewportNavigationSettingsStore& Get();

    const ViewportNavigationSettings& GetSettings() const { return m_Settings; }
    ViewportNavigationSettings& GetMutableSettings() { return m_Settings; }

    void EnsureLoaded();
    void Save() const;
    void ApplyPreset(NavigationPreset preset);

    static std::string PresetToString(NavigationPreset preset);
    static NavigationPreset PresetFromString(const std::string& value);

private:
    ViewportNavigationSettingsStore() = default;

    void Load();
    ViewportNavigationSettings m_Settings{};
    bool m_Loaded = false;
};

} // namespace we::programs::editor
