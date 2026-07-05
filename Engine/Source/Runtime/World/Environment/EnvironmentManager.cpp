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

glm::vec3 EnvironmentManager::GetWorldOrigin(const glm::vec3& /*cameraPosition*/) const {
    // Fixed world origin keeps sky/fog/cloud math stable while the editor camera moves.
    return glm::vec3(0.0f);
}

glm::vec3 EnvironmentManager::ComputeSkyLightUpper(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 rayleigh = atmosphere.GetRayleighColor();
    const glm::vec3 sunColor = sun.GetColorFromTemperature();

    // Procedural capture: scattered skylight grows with sun elevation and Rayleigh coefficients.
    const glm::vec3 scattered = rayleigh * (8.0f + elevation * 40.0f);
    const glm::vec3 sunBounce = sunColor * sun.Intensity * 0.02f * (1.0f - elevation);
    const float night = Clamp01(0.2f - elevation);
    return scattered + sunBounce + rayleigh * night * 0.5f;
}

glm::vec3 EnvironmentManager::ComputeSkyLightLower(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentHeightFog& fog,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const float night = Clamp01(1.0f - elevation * 2.5f);
    const glm::vec3 groundScatter = atmosphere.GetRayleighColor() * 0.8f;
    return glm::mix(fog.FogColor * 0.4f + groundScatter, groundScatter * 0.15f, night);
}

glm::vec3 EnvironmentManager::ComputeFogColor(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 sunColor = sun.GetColorFromTemperature();
    const glm::vec3 rayleigh = atmosphere.GetRayleighColor();

    const glm::vec3 dayFog = rayleigh * 6.0f + sunColor * 0.08f;
    const glm::vec3 sunsetFog = sunColor * 0.35f + rayleigh * 3.0f;
    const glm::vec3 nightFog = rayleigh * 0.4f;

    glm::vec3 fogColor = glm::mix(sunsetFog, dayFog, elevation);
    fogColor = glm::mix(nightFog, fogColor, Clamp01(elevation * 2.0f + 0.1f));
    return fogColor;
}

float EnvironmentManager::ComputeExposureEV(const EnvironmentDirectionalLight& sun) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = sunDir.y;
    const float dayFactor = Clamp01(elevation * 2.5f + 0.1f);
    const float twilightFactor = Clamp01(1.0f - abs(elevation) * 3.5f);
    constexpr float kDayEV = 8.0f;
    constexpr float kNightEV = 1.0f;
    constexpr float kTwilightEV = 4.0f;
    float ev = glm::mix(kNightEV, kDayEV, dayFactor);
    ev = glm::mix(ev, kTwilightEV, twilightFactor * (1.0f - dayFactor));
    return ev;
}

float EnvironmentManager::ComputeHdrSkyLuminance(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = SunDirectionToSky(sun.GetLightDirection());
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 upper = ComputeSkyLightUpper(sun, atmosphere);
    const float sunDisk = sun.Intensity * Clamp01(elevation * 3.0f + 0.05f);
    // Match order-of-magnitude of analytic sky HDR (WE_SKY_RADIANCE_SCALE in shaders).
    constexpr float kShaderRadianceScale = 6.0f;
    const float cpuKey = glm::dot(upper, glm::vec3(0.2126f, 0.7152f, 0.0722f)) + sunDisk * 0.15f;
    return std::max(cpuKey * kShaderRadianceScale * 1.35f, 0.5f);
}

void EnvironmentManager::UpdateDerivedState(
    EnvironmentDirectionalLight& sun,
    EnvironmentSkyLight& skyLight,
    EnvironmentHeightFog& fog,
    EnvironmentSkyAtmosphere& atmosphere,
    const glm::vec3& cameraPosition) {
    sun.Color = sun.GetColorFromTemperature();

    const glm::vec3 worldOrigin = GetWorldOrigin(cameraPosition);
    const float altitudeMeters = std::max(cameraPosition.y - worldOrigin.y, 0.0f);
    atmosphere.EyeAltitude = std::max(altitudeMeters * 0.001f, 0.001f);

    if (skyLight.RealTimeCapture) {
        skyLight.UpperHemisphereColor = ComputeSkyLightUpper(sun, atmosphere);
        skyLight.LowerHemisphereColor = ComputeSkyLightLower(sun, fog, atmosphere);
    }

    fog.FogColor = ComputeFogColor(sun, atmosphere);
}

} // namespace we::runtime::world::environment
