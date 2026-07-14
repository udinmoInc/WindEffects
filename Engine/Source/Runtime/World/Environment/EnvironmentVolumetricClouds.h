#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "World/Export.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

enum class CloudQualityPreset {
    Low = 0,
    Medium = 1,
    High = 2,
    Epic = 3
};

enum class CloudPreset {
    ClearSky,
    FewClouds,
    ScatteredClouds,
    BrokenClouds,
    Overcast,
    Storm,
    HeavyStorm,
    SunsetClouds,
    SunriseClouds,
    HighCirrus,
    Cumulus,
    Stratocumulus
};

// Global volumetric cloud component (one active instance per level).
class WORLD_API EnvironmentVolumetricClouds {
public:
    std::uint64_t EntityId = 0;

    bool Enabled = true;

    float Coverage = 0.45f;
    float Density = 1.0f;
    float DensityMultiplier = 1.0f;
    float CloudHeight = 5000.0f;      // layer center (m) — alias of Altitude
    float CloudThickness = 800.0f;    // meters
    float BottomAltitude = 4600.0f;
    float TopAltitude = 5400.0f;
    float Altitude = 5000.0f;         // kept for settings/IO compatibility
    float Extinction = 0.35f;

    glm::vec3 WindDirection{ 1.0f, 0.0f, 0.25f };
    float WindSpeed = 12.0f;
    float AnimationSpeed = 1.0f;
    float NoiseScale = 1.0f;
    float DetailNoiseScale = 3.7f;
    float ShapeNoise = 0.75f;
    float ErosionNoise = 0.25f;
    float Seed = 17.3f;

    float LightingIntensity = 1.0f;
    float SilverLiningIntensity = 0.85f;
    float AmbientContribution = 0.42f;
    float MultiScatteringStrength = 0.35f;
    float PhaseG = 0.6f;              // forward scattering anisotropy
    float PowderEffect = 0.5f;
    float WeatherMapInfluence = 0.0f; // reserved for future weather maps

    float ShadowStrength = 0.65f;
    float ShadowDistance = 4000.0f;
    int ShadowResolution = 512;

    glm::vec3 CloudColor{ 0.95f, 0.96f, 0.98f };
    glm::vec3 CloudColorTint{ 1.0f, 1.0f, 1.0f };

    CloudQualityPreset Quality = CloudQualityPreset::Medium;
    CloudPreset ActivePreset = CloudPreset::ScatteredClouds;

    // Runtime animation clock (seconds). Advanced by EnvironmentSystem.
    float AnimationTime = 0.0f;

    void ApplyDefaults();
    void ApplyPreset(CloudPreset preset);
    void SyncAltitudeFromBounds();
    void Tick(float deltaTime);

    static const char* PresetName(CloudPreset preset);
    static int RaymarchStepsForQuality(CloudQualityPreset quality);
};

} // namespace we::runtime::world::environment

#pragma warning(pop)
