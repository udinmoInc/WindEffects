// ==============================================================================
// WindEffects — World — DefaultSceneSettings
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <filesystem>
#include <string>

namespace we::runtime::world {

struct DefaultSceneSettings {
    bool enableDefaultScene = true;
    bool createDirectionalLight = true;
    bool createSkyLight = true;
    bool createSkyAtmosphere = true;
    bool createFog = true;
    bool createGroundPlane = false;

    float sunIntensity = 10.0f;
    int sunTemperature = 6500;
    float sunRotationPitch = -45.0f;
    float sunRotationYaw = 35.0f;

    float skyIntensity = 1.0f;
    bool skyRealTimeCapture = true;

    float fogDensity = 0.01f;
};

class DefaultSceneSettingsLoader {
public:
    static DefaultSceneSettingsLoader& Get();

    const DefaultSceneSettings& GetSettings();

private:
    std::filesystem::path GetConfigPath() const;
    void EnsureLoaded();
    void EnsureConfigHasDefaults(const std::filesystem::path& path, const DefaultSceneSettings& defaults) const;
    DefaultSceneSettings ParseSettings(const std::filesystem::path& path, const DefaultSceneSettings& defaults) const;

    DefaultSceneSettings m_Settings{};
    bool m_Loaded = false;
};

} // namespace we::runtime::world
