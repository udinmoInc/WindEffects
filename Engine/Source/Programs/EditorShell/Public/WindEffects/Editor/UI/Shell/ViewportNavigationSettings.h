// ==============================================================================
// WindEffects — EditorShell — ViewportNavigationSettings
// Public API surface for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <string>

namespace we::editor::viewport {

enum class NavigationPreset {
    UE5,
    Blender,
    Maya,
    Unity,
    Custom
};

// Viewport navigation settings, persisted across editor sessions.
struct ViewportNavigationSettings {
    NavigationPreset preset = NavigationPreset::UE5;

    float mouseSensitivity = 0.25f;
    float cameraAcceleration = 1.0f;
    float cameraSmoothing = 45.0f;
    bool invertX = false;
    bool invertY = false;

    float defaultCameraSpeed = 4.0f;
    float maxBoostMultiplier = 4.0f;
    float slowMultiplier = 0.25f;

    bool orbitAroundSelection = true;
    bool focusOnSelection = true;
    float scrollWheelSpeedMultiplier = 1.0f;
};

class EDITORSHELL_API ViewportNavigationSettingsStore {
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

} // namespace we::editor::viewport
