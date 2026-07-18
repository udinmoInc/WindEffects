#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "World/Export.h"

#include <cstdint>
#include "Core/Math/Types.h"

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

    float Coverage = 0.55f;
    float Density = 1.15f;
    float DensityMultiplier = 1.0f;
    float CloudHeight = 1250.0f;      // layer center (m) â€” alias of Altitude
    float CloudThickness = 700.0f;    // meters
    float BottomAltitude = 900.0f;
    float TopAltitude = 1600.0f;
    float Altitude = 1250.0f;         // kept for settings/IO compatibility
    float Extinction = 0.55f;

    we::math::Vec3 WindDirection{ 1.0f, 0.0f, 0.25f };
    float WindSpeed = 18.0f;
    float AnimationSpeed = 1.0f;
    float NoiseScale = 1.15f;
    float DetailNoiseScale = 4.2f;
    float ShapeNoise = 0.85f;
    float ErosionNoise = 0.32f;
    float Seed = 17.3f;

    float LightingIntensity = 1.0f;
    float SilverLiningIntensity = 0.9f;
    float AmbientContribution = 0.45f;
    float MultiScatteringStrength = 0.4f;
    float PhaseG = 0.6f;              // forward scattering anisotropy
    float PowderEffect = 0.5f;
    float WeatherMapInfluence = 0.0f; // reserved for future weather maps

    float ShadowStrength = 0.65f;
    float ShadowDistance = 4000.0f;
    int ShadowResolution = 512;

    we::math::Vec3 CloudColor{ 0.95f, 0.96f, 0.98f };
    we::math::Vec3 CloudColorTint{ 1.0f, 1.0f, 1.0f };

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
