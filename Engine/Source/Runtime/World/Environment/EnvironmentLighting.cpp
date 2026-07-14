#include "Environment/EnvironmentLighting.h"

#include "Environment/EnvironmentManager.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

float SrgbChannelToLinear(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

glm::vec3 SrgbToLinear(const glm::vec3& srgb) {
    return glm::vec3(
        SrgbChannelToLinear(srgb.x),
        SrgbChannelToLinear(srgb.y),
        SrgbChannelToLinear(srgb.z));
}

} // namespace

glm::vec3 TemperatureKelvinToRgb(int kelvin) {
    const float temp = std::clamp(static_cast<float>(kelvin), 1000.0f, 40000.0f) / 100.0f;

    float red = 0.0f;
    if (temp <= 66.0f) {
        red = 255.0f;
    } else {
        red = temp - 60.0f;
        red = 329.698727446f * std::pow(red, -0.1332047592f);
    }

    float green = 0.0f;
    if (temp <= 66.0f) {
        green = temp;
        green = 99.4708025861f * std::log(green) - 161.1195681661f;
    } else {
        green = temp - 60.0f;
        green = 288.1221695283f * std::pow(green, -0.0755148492f);
    }

    float blue = 0.0f;
    if (temp >= 66.0f) {
        blue = 255.0f;
    } else if (temp <= 19.0f) {
        blue = 0.0f;
    } else {
        blue = temp - 10.0f;
        blue = 138.5177312231f * std::log(blue) - 305.0447927307f;
    }

    const glm::vec3 srgb(
        std::clamp(red / 255.0f, 0.0f, 1.0f),
        std::clamp(green / 255.0f, 0.0f, 1.0f),
        std::clamp(blue / 255.0f, 0.0f, 1.0f));
    return SrgbToLinear(srgb);
}

glm::vec3 EulerDegreesToLightDirection(const glm::vec3& rotationDegrees) {
    const float pitch = glm::radians(rotationDegrees.x);
    const float yaw = glm::radians(rotationDegrees.y);

    glm::vec3 direction;
    // Rotation pitch follows editor convention: negative pitch = sun above horizon,
    // light travels toward the scene (downward in world +Y-up space).
    direction.x = std::cos(pitch) * std::sin(yaw);
    direction.y = std::sin(pitch);
    direction.z = std::cos(pitch) * std::cos(yaw);
    return glm::normalize(direction);
}

glm::vec3 SunDirectionToSky(const glm::vec3& lightTravelDirection) {
    return glm::normalize(-lightTravelDirection);
}

we::runtime::renderer::SceneEnvironmentUniform BuildSceneEnvironmentUniform(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyLight& skyLight,
    const EnvironmentSkyAtmosphere& atmosphere,
    const EnvironmentHeightFog& fog,
    const EnvironmentVolumetricClouds& clouds,
    const EnvironmentExposureController& exposure,
    const glm::vec3& worldOrigin) {

    EnvironmentManager manager;
    const float sunDerivedEV = manager.ComputeExposureEV(sun);

    // Artist intensity (~10 daytime) maps to display-referred outdoor irradiance.
    constexpr float kSunArtistToIrradiance = 0.12f;

    we::runtime::renderer::SceneEnvironmentUniform uniform{};
    uniform.sunDirection = sun.GetLightDirection();
    uniform.sunIntensity = sun.Intensity * kSunArtistToIrradiance;
    uniform.sunColor = sun.GetColorFromTemperature();
    uniform.skyLightIntensity = skyLight.Intensity;
    uniform.skyAmbientColor = skyLight.GetAmbientColor();
    uniform.skyLightLowerColor = skyLight.LowerHemisphereColor;
    uniform.fogDensity = fog.Density;
    uniform.fogColor = fog.FogColor;
    uniform.fogHeightFalloff = fog.HeightFalloff;
    uniform.fogStartDistance = fog.StartDistance;
    uniform.atmosphereRayleigh = atmosphere.GetRayleighColor();
    uniform.mieScattering = atmosphere.MieScattering;
    uniform.ozoneAbsorption = atmosphere.GetOzoneAbsorption();
    uniform.mieAnisotropy = atmosphere.MieAnisotropy;
    uniform.worldOrigin = worldOrigin;
    uniform.exposureEV = exposure.AutoExposure
        ? std::clamp(exposure.ExposureEV, exposure.MinEV, exposure.MaxEV)
        : exposure.GetEffectiveExposureEV(sunDerivedEV);
    uniform.planetRadius = 6360.0f;
    uniform.atmosphereHeight = 60.0f;
    uniform.multiScatterStrength = atmosphere.MultiScatterStrength;
    uniform.eyeAltitude = atmosphere.EyeAltitude;

    EnvironmentVolumetricClouds cloudState = clouds;
    cloudState.SyncAltitudeFromBounds();

    const bool cloudsActive = cloudState.Enabled && cloudState.EntityId != 0;
    uniform.cloudCoverage = cloudState.Coverage;
    uniform.cloudAltitude = cloudState.Altitude;
    uniform.cloudExtinction = cloudState.Extinction;
    uniform.enableClouds = cloudsActive ? 1.0f : 0.0f;
    uniform.cloudColor = cloudState.CloudColor * cloudState.CloudColorTint;
    uniform.enableVolumetricFog = fog.VolumetricFog ? 1.0f : 0.0f;
    uniform.exposureCompensation = exposure.ExposureCompensation;
    uniform.sunAngularRadius = 0.004675f;
    uniform.hdrSkyLuminance = manager.ComputeHdrSkyLuminance(sun, atmosphere);
    uniform.sunCastShadows = sun.CastDynamicShadows ? 1 : 0;
    uniform.sunTemperature = sun.TemperatureKelvin;
    uniform.bloomIntensity = 0.15f;
    uniform.bloomThreshold = 4.0f;
    uniform.enableAutoExposure = exposure.AutoExposure ? 1.0f : 0.0f;
    uniform.atmosphereDebugMode = atmosphere.AtmosphereDebugMode;
    uniform.cloudTemporalBlend = 0.88f;
    uniform.cloudHistoryValid = 0;
    uniform.enableSunDisk = 1.0f;
    uniform.pipelineFixedExposureMultiplier = 0.0f;

    uniform.cloudDensityMult = cloudState.Density * cloudState.DensityMultiplier;
    uniform.cloudThickness = cloudState.CloudThickness;
    uniform.cloudBottomAltitude = cloudState.BottomAltitude;
    uniform.cloudTopAltitude = cloudState.TopAltitude;
    uniform.cloudWindDir = glm::length(cloudState.WindDirection) > 1e-4f
        ? glm::normalize(cloudState.WindDirection)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    uniform.cloudWindSpeed = cloudState.WindSpeed;
    uniform.cloudNoiseScale = cloudState.NoiseScale;
    uniform.cloudDetailScale = cloudState.DetailNoiseScale;
    uniform.cloudLightingIntensity = cloudState.LightingIntensity;
    uniform.cloudSilverLining = cloudState.SilverLiningIntensity;
    uniform.cloudAmbient = cloudState.AmbientContribution;
    uniform.cloudMultiScatter = cloudState.MultiScatteringStrength;
    uniform.cloudPhaseG = cloudState.PhaseG;
    uniform.cloudPowder = cloudState.PowderEffect;
    uniform.cloudSeed = cloudState.Seed;
    uniform.cloudAnimTime = cloudState.AnimationTime;
    uniform.cloudShadowStrength = cloudState.ShadowStrength;
    uniform.cloudQualitySteps = EnvironmentVolumetricClouds::RaymarchStepsForQuality(cloudState.Quality);
    uniform.cloudShapeNoise = cloudState.ShapeNoise;
    uniform.cloudErosionNoise = cloudState.ErosionNoise;
    (void)cloudState.WeatherMapInfluence;
    (void)cloudState.ShadowDistance;
    (void)cloudState.ShadowResolution;

    return uniform;
}

} // namespace we::runtime::world::environment
