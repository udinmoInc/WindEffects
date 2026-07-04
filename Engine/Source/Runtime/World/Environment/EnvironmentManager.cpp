#include "Environment/EnvironmentManager.h"

#include "Environment/EnvironmentLighting.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

glm::vec3 EnvironmentManager::GetWorldOrigin(const glm::vec3& cameraPosition) const {
    // Large-world rebasing: snap origin to 1 km grid so fog/aerial math stays stable.
    constexpr float kRebaseGrid = 1000.0f;
    return glm::vec3(
        std::floor(cameraPosition.x / kRebaseGrid) * kRebaseGrid,
        std::floor(cameraPosition.y / kRebaseGrid) * kRebaseGrid,
        std::floor(cameraPosition.z / kRebaseGrid) * kRebaseGrid);
}

glm::vec3 EnvironmentManager::ComputeSkyLightUpper(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);

    const glm::vec3 rayleigh = atmosphere.GetRayleighColor();
    const glm::vec3 zenith = glm::vec3(0.18f, 0.38f, 0.95f) * rayleigh * 18.0f;
    const glm::vec3 dayZenith = zenith * glm::mix(0.12f, 1.0f, elevation);

    const glm::vec3 sunColor = sun.GetColorFromTemperature();
    const glm::vec3 sunsetWarm = sunColor * sun.Intensity * 0.035f * (1.0f - elevation);

    const float nightFactor = Clamp01(0.35f - elevation);
    const glm::vec3 nightTint = glm::vec3(0.02f, 0.03f, 0.06f) * nightFactor;

    return dayZenith + sunsetWarm + nightTint;
}

glm::vec3 EnvironmentManager::ComputeSkyLightLower(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentHeightFog& fog) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const float nightFactor = Clamp01(1.0f - elevation * 2.0f);
    return glm::mix(fog.FogColor * 0.35f, glm::vec3(0.02f, 0.025f, 0.04f), nightFactor) * (0.5f + fog.Density);
}

glm::vec3 EnvironmentManager::ComputeFogColor(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 sunColor = sun.GetColorFromTemperature();

    const glm::vec3 horizonBase = glm::vec3(0.72f, 0.78f, 0.85f) + atmosphere.GetRayleighColor() * 4.0f;
    const glm::vec3 sunsetFog = sunColor * glm::mix(0.55f, 0.15f, elevation);
    const glm::vec3 nightFog = glm::vec3(0.04f, 0.05f, 0.08f);

    glm::vec3 fogColor = glm::mix(horizonBase, sunsetFog, glm::pow(1.0f - elevation, 2.0f));
    fogColor = glm::mix(fogColor, nightFog, Clamp01(0.25f - elevation));
    return fogColor;
}

float EnvironmentManager::ComputeExposureEV(const EnvironmentDirectionalLight& sun) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y * 2.5f + 0.15f);
    constexpr float kDayEV = 2.35f;
    constexpr float kNightEV = -2.15f;
    return glm::mix(kNightEV, kDayEV + 1.2f, elevation);
}

void EnvironmentManager::UpdateDerivedState(
    EnvironmentDirectionalLight& sun,
    EnvironmentSkyLight& skyLight,
    EnvironmentHeightFog& fog,
    EnvironmentSkyAtmosphere& atmosphere,
    const glm::vec3& cameraPosition) {
    (void)cameraPosition;

    sun.Color = sun.GetColorFromTemperature();

    if (skyLight.RealTimeCapture) {
        skyLight.UpperHemisphereColor = ComputeSkyLightUpper(sun, atmosphere);
        skyLight.LowerHemisphereColor = ComputeSkyLightLower(sun, fog);
    }

    fog.FogColor = ComputeFogColor(sun, atmosphere);
}

} // namespace we::runtime::world::environment
