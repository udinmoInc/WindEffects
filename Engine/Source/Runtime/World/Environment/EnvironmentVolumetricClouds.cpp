#include "Environment/EnvironmentVolumetricClouds.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

void Clamp01(float& v) {
    v = std::clamp(v, 0.0f, 1.0f);
}

void SetLayer(EnvironmentVolumetricClouds& clouds, float bottom, float top) {
    clouds.BottomAltitude = bottom;
    clouds.TopAltitude = top;
    clouds.SyncAltitudeFromBounds();
}

} // namespace

void EnvironmentVolumetricClouds::ApplyDefaults() {
    Enabled = true;
    Coverage = 0.55f;
    Density = 1.15f;
    DensityMultiplier = 1.0f;
    // Editor default: low-alt cumulus so camera at ~6m looks through the sky lobe and sees clouds.
    CloudThickness = 700.0f;
    BottomAltitude = 900.0f;
    TopAltitude = 1600.0f;
    CloudHeight = 1250.0f;
    Altitude = 1250.0f;
    Extinction = 0.55f;
    WindDirection = glm::vec3(1.0f, 0.0f, 0.25f);
    WindSpeed = 18.0f;
    AnimationSpeed = 1.0f;
    NoiseScale = 1.15f;
    DetailNoiseScale = 4.2f;
    ShapeNoise = 0.85f;
    ErosionNoise = 0.32f;
    Seed = 17.3f;
    LightingIntensity = 1.0f;
    SilverLiningIntensity = 0.9f;
    AmbientContribution = 0.45f;
    MultiScatteringStrength = 0.4f;
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
        Coverage = 0.28f;
        Density = 0.85f;
        SetLayer(*this, 1000.0f, 1500.0f);
        break;
    case CloudPreset::ScatteredClouds:
        Coverage = 0.55f;
        Density = 1.15f;
        SetLayer(*this, 900.0f, 1600.0f);
        break;
    case CloudPreset::BrokenClouds:
        Coverage = 0.7f;
        Density = 1.25f;
        Extinction = 0.6f;
        SetLayer(*this, 800.0f, 1700.0f);
        break;
    case CloudPreset::Overcast:
        Coverage = 0.9f;
        Density = 1.4f;
        Extinction = 0.7f;
        AmbientContribution = 0.55f;
        LightingIntensity = 0.75f;
        SetLayer(*this, 700.0f, 1800.0f);
        break;
    case CloudPreset::Storm:
        Coverage = 0.82f;
        Density = 1.5f;
        Extinction = 0.8f;
        CloudColor = glm::vec3(0.72f, 0.74f, 0.78f);
        WindSpeed = 35.0f;
        ShadowStrength = 0.85f;
        SetLayer(*this, 500.0f, 2000.0f);
        break;
    case CloudPreset::HeavyStorm:
        Coverage = 0.94f;
        Density = 1.75f;
        Extinction = 0.95f;
        CloudColor = glm::vec3(0.55f, 0.57f, 0.62f);
        WindSpeed = 48.0f;
        AnimationSpeed = 1.35f;
        ShadowStrength = 0.95f;
        LightingIntensity = 0.55f;
        SetLayer(*this, 400.0f, 2200.0f);
        break;
    case CloudPreset::SunsetClouds:
        Coverage = 0.55f;
        Density = 1.1f;
        CloudColorTint = glm::vec3(1.15f, 0.75f, 0.55f);
        SilverLiningIntensity = 1.25f;
        LightingIntensity = 1.1f;
        SetLayer(*this, 950.0f, 1650.0f);
        break;
    case CloudPreset::SunriseClouds:
        Coverage = 0.52f;
        Density = 1.05f;
        CloudColorTint = glm::vec3(1.05f, 0.82f, 0.7f);
        SilverLiningIntensity = 1.15f;
        SetLayer(*this, 950.0f, 1600.0f);
        break;
    case CloudPreset::HighCirrus:
        Coverage = 0.4f;
        Density = 0.5f;
        Extinction = 0.22f;
        NoiseScale = 0.55f;
        DetailNoiseScale = 5.5f;
        WindSpeed = 40.0f;
        SetLayer(*this, 5500.0f, 6200.0f);
        break;
    case CloudPreset::Cumulus:
        Coverage = 0.5f;
        Density = 1.35f;
        Extinction = 0.6f;
        ShapeNoise = 0.88f;
        ErosionNoise = 0.35f;
        SilverLiningIntensity = 1.0f;
        SetLayer(*this, 850.0f, 1800.0f);
        break;
    case CloudPreset::Stratocumulus:
        Coverage = 0.72f;
        Density = 1.15f;
        Extinction = 0.5f;
        NoiseScale = 0.85f;
        WindSpeed = 22.0f;
        SetLayer(*this, 600.0f, 1100.0f);
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
