#pragma once

#include "EnvironmentTypes.h"
#include <filesystem>

namespace we::runtime::world::environment {

struct EnvironmentSettings {
    bool autoCreateOnNewLevel = true;
    bool createDirectionalLight = true;
    bool createSkyLight = true;
    bool createSkyAtmosphere = true;
    bool createHeightFog = true;
    bool createVolumetricClouds = false;
    bool enableVolumetricFog = true;

    float sunIntensity = 10.0f;
    int sunTemperature = 6500;
    float sunRotationPitch = -45.0f;
    float sunRotationYaw = 35.0f;
    bool sunCastShadows = true;

    float skyLightIntensity = 1.0f;
    bool skyLightRealTimeCapture = true;

    float atmosphereMieScattering = 0.003996f;
    float atmosphereRayleighScattering = 0.005802f;

    float fogDensity = 0.02f;
    float fogHeightFalloff = 0.2f;
    float fogStartDistance = 0.0f;

    float cloudCoverage = 0.45f;
    float cloudAltitude = 5000.0f;
};

class EnvironmentSettingsLoader {
public:
    static EnvironmentSettingsLoader& Get();

    const EnvironmentSettings& GetSettings();
    void SaveSettings(const EnvironmentSettings& settings);

private:
    void EnsureLoaded();
    std::filesystem::path GetConfigPath() const;

    EnvironmentSettings m_Settings{};
    bool m_Loaded = false;
};

} // namespace we::runtime::world::environment
