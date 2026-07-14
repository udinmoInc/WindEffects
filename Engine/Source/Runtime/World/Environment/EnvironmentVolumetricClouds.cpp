#include "Environment/EnvironmentVolumetricClouds.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

void Clamp01(float& v) {
    v = std::clamp(v, 0.0f, 1.0f);
}

} // namespace

void EnvironmentVolumetricClouds::ApplyDefaults() {
    Enabled = true;
    Coverage = 0.45f;
    Density = 1.0f;
    DensityMultiplier = 1.0f;
    CloudHeight = 5000.0f;
    CloudThickness = 800.0f;
    BottomAltitude = 4600.0f;
    TopAltitude = 5400.0f;
    Altitude = 5000.0f;
    Extinction = 0.35f;
    WindDirection = glm::vec3(1.0f, 0.0f, 0.25f);
    WindSpeed = 12.0f;
    AnimationSpeed = 1.0f;
    NoiseScale = 1.0f;
    DetailNoiseScale = 3.7f;
    ShapeNoise = 0.75f;
    ErosionNoise = 0.25f;
    Seed = 17.3f;
    LightingIntensity = 1.0f;
    SilverLiningIntensity = 0.85f;
    AmbientContribution = 0.42f;
    MultiScatteringStrength = 0.35f;
    PhaseG = 0.6f;
    PowderEffect = 0.5f;
    WeatherMapInfluence = 0.0f;
    ShadowStrength = 0.65f;
    ShadowDistance = 4000.0f;
    ShadowResolution = 512;
    CloudColor = glm::vec3(0.95f, 0.96f, 0.98f);
    CloudColorTint = glm::vec3(1.0f, 1.0f, 1.0f);
    Quality = CloudQualityPreset::Medium;
    ActivePreset = CloudPreset::ScatteredClouds;
    AnimationTime = 0.0f;
}

void EnvironmentVolumetricClouds::SyncAltitudeFromBounds() {
    if (TopAltitude < BottomAltitude) {
        std::swap(TopAltitude, BottomAltitude);
    }
    CloudThickness = std::max(50.0f, TopAltitude - BottomAltitude);
    CloudHeight = 0.5f * (TopAltitude + BottomAltitude);
    Altitude = CloudHeight;
}

void EnvironmentVolumetricClouds::Tick(float deltaTime) {
    if (!Enabled || deltaTime <= 0.0f) {
        return;
    }
    AnimationTime += deltaTime * AnimationSpeed;
    // Keep deterministic wrap for GPU float stability.
    constexpr float kWrap = 3600.0f;
    if (AnimationTime > kWrap) {
        AnimationTime = std::fmod(AnimationTime, kWrap);
    }
}

int EnvironmentVolumetricClouds::RaymarchStepsForQuality(CloudQualityPreset quality) {
    switch (quality) {
    case CloudQualityPreset::Low: return 16;
    case CloudQualityPreset::High: return 48;
    case CloudQualityPreset::Epic: return 64;
    case CloudQualityPreset::Medium:
    default: return 32;
    }
}

const char* EnvironmentVolumetricClouds::PresetName(CloudPreset preset) {
    switch (preset) {
    case CloudPreset::ClearSky: return "Clear Sky";
    case CloudPreset::FewClouds: return "Few Clouds";
    case CloudPreset::ScatteredClouds: return "Scattered Clouds";
    case CloudPreset::BrokenClouds: return "Broken Clouds";
    case CloudPreset::Overcast: return "Overcast";
    case CloudPreset::Storm: return "Storm";
    case CloudPreset::HeavyStorm: return "Heavy Storm";
    case CloudPreset::SunsetClouds: return "Sunset Clouds";
    case CloudPreset::SunriseClouds: return "Sunrise Clouds";
    case CloudPreset::HighCirrus: return "High Cirrus";
    case CloudPreset::Cumulus: return "Cumulus";
    case CloudPreset::Stratocumulus: return "Stratocumulus";
    default: return "Scattered Clouds";
    }
}

void EnvironmentVolumetricClouds::ApplyPreset(CloudPreset preset) {
    ApplyDefaults();
    ActivePreset = preset;

    switch (preset) {
    case CloudPreset::ClearSky:
        Enabled = false;
        Coverage = 0.05f;
        Density = 0.2f;
        break;
    case CloudPreset::FewClouds:
        Coverage = 0.22f;
        Density = 0.7f;
        CloudThickness = 600.0f;
        BottomAltitude = 4700.0f;
        TopAltitude = 5300.0f;
        break;
    case CloudPreset::ScatteredClouds:
        Coverage = 0.45f;
        Density = 1.0f;
        CloudThickness = 800.0f;
        BottomAltitude = 4600.0f;
        TopAltitude = 5400.0f;
        break;
    case CloudPreset::BrokenClouds:
        Coverage = 0.62f;
        Density = 1.15f;
        Extinction = 0.42f;
        CloudThickness = 1000.0f;
        BottomAltitude = 4400.0f;
        TopAltitude = 5400.0f;
        break;
    case CloudPreset::Overcast:
        Coverage = 0.88f;
        Density = 1.3f;
        Extinction = 0.55f;
        AmbientContribution = 0.55f;
        CloudThickness = 1400.0f;
        BottomAltitude = 4000.0f;
        TopAltitude = 5400.0f;
        LightingIntensity = 0.75f;
        break;
    case CloudPreset::Storm:
        Coverage = 0.78f;
        Density = 1.45f;
        Extinction = 0.7f;
        CloudColor = glm::vec3(0.72f, 0.74f, 0.78f);
        WindSpeed = 28.0f;
        CloudThickness = 1800.0f;
        BottomAltitude = 3500.0f;
        TopAltitude = 5300.0f;
        ShadowStrength = 0.85f;
        break;
    case CloudPreset::HeavyStorm:
        Coverage = 0.92f;
        Density = 1.7f;
        Extinction = 0.9f;
        CloudColor = glm::vec3(0.55f, 0.57f, 0.62f);
        WindSpeed = 40.0f;
        AnimationSpeed = 1.35f;
        CloudThickness = 2200.0f;
        BottomAltitude = 2800.0f;
        TopAltitude = 5000.0f;
        ShadowStrength = 0.95f;
        LightingIntensity = 0.55f;
        break;
    case CloudPreset::SunsetClouds:
        Coverage = 0.5f;
        Density = 1.05f;
        CloudColorTint = glm::vec3(1.15f, 0.75f, 0.55f);
        SilverLiningIntensity = 1.25f;
        LightingIntensity = 1.1f;
        CloudThickness = 900.0f;
        BottomAltitude = 4500.0f;
        TopAltitude = 5400.0f;
        break;
    case CloudPreset::SunriseClouds:
        Coverage = 0.48f;
        Density = 1.0f;
        CloudColorTint = glm::vec3(1.05f, 0.82f, 0.7f);
        SilverLiningIntensity = 1.15f;
        CloudThickness = 850.0f;
        BottomAltitude = 4550.0f;
        TopAltitude = 5400.0f;
        break;
    case CloudPreset::HighCirrus:
        Coverage = 0.35f;
        Density = 0.45f;
        Extinction = 0.18f;
        CloudThickness = 350.0f;
        BottomAltitude = 9000.0f;
        TopAltitude = 9350.0f;
        NoiseScale = 0.55f;
        DetailNoiseScale = 5.5f;
        WindSpeed = 35.0f;
        break;
    case CloudPreset::Cumulus:
        Coverage = 0.4f;
        Density = 1.25f;
        Extinction = 0.48f;
        ShapeNoise = 0.85f;
        ErosionNoise = 0.35f;
        CloudThickness = 1200.0f;
        BottomAltitude = 4200.0f;
        TopAltitude = 5400.0f;
        SilverLiningIntensity = 1.0f;
        break;
    case CloudPreset::Stratocumulus:
        Coverage = 0.68f;
        Density = 1.1f;
        Extinction = 0.4f;
        CloudThickness = 500.0f;
        BottomAltitude = 1800.0f;
        TopAltitude = 2300.0f;
        NoiseScale = 0.8f;
        WindSpeed = 18.0f;
        break;
    }

    SyncAltitudeFromBounds();
    Clamp01(Coverage);
    Clamp01(ShapeNoise);
    Clamp01(ErosionNoise);
    Clamp01(PowderEffect);
    Clamp01(WeatherMapInfluence);
}

} // namespace we::runtime::world::environment
