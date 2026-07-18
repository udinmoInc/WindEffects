#include "Environment/EnvironmentManager.h"

#include "Environment/EnvironmentLighting.h"
#include "Core/Math/GlmInterop.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

we::math::Vec3 EnvironmentManager::GetWorldOrigin(const we::math::Vec3& /*cameraPosition*/) const {
    return we::math::Vec3(0.0f);
}

we::math::Vec3 EnvironmentManager::ComputeSkyLightUpper(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 rayleigh = we::math::ToGlm(atmosphere.GetRayleighColor());
    const glm::vec3 sunColor = we::math::ToGlm(sun.GetColorFromTemperature());

    const glm::vec3 scattered = rayleigh * (8.0f + elevation * 40.0f);
    const glm::vec3 sunBounce = sunColor * sun.Intensity * 0.02f * (1.0f - elevation);
    const float night = Clamp01(0.2f - elevation);
    return we::math::FromGlm(scattered + sunBounce + rayleigh * night * 0.5f);
}

we::math::Vec3 EnvironmentManager::ComputeSkyLightLower(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentHeightFog& fog,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const float night = Clamp01(1.0f - elevation * 2.5f);
    const glm::vec3 groundScatter = we::math::ToGlm(atmosphere.GetRayleighColor()) * 0.8f;
    const glm::vec3 fogTint = we::math::ToGlm(fog.FogColor) * 0.4f + groundScatter;
    return we::math::FromGlm(glm::mix(fogTint, groundScatter * 0.15f, night));
}

we::math::Vec3 EnvironmentManager::ComputeFogColor(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 sunColor = we::math::ToGlm(sun.GetColorFromTemperature());
    const glm::vec3 rayleigh = we::math::ToGlm(atmosphere.GetRayleighColor());

    const glm::vec3 dayFog = rayleigh * 6.0f + sunColor * 0.08f;
    const glm::vec3 sunsetFog = sunColor * 0.35f + rayleigh * 3.0f;
    const glm::vec3 nightFog = rayleigh * 0.4f;

    glm::vec3 fogColor = glm::mix(sunsetFog, dayFog, elevation);
    fogColor = glm::mix(nightFog, fogColor, Clamp01(elevation * 2.0f + 0.1f));
    return we::math::FromGlm(fogColor);
}

float EnvironmentManager::ComputeExposureEV(const EnvironmentDirectionalLight& sun) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = sunDir.y;
    const float dayFactor = Clamp01(elevation * 2.5f + 0.1f);
    const float twilightFactor = Clamp01(1.0f - std::abs(elevation) * 3.5f);
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
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 upper = we::math::ToGlm(ComputeSkyLightUpper(sun, atmosphere));
    const float sunDisk = sun.Intensity * Clamp01(elevation * 3.0f + 0.05f);
    constexpr float kShaderRadianceScale = 6.0f;
    const float cpuKey = glm::dot(upper, glm::vec3(0.2126f, 0.7152f, 0.0722f)) + sunDisk * 0.15f;
    return std::max(cpuKey * kShaderRadianceScale * 1.35f, 0.5f);
}

void EnvironmentManager::UpdateDerivedState(
    EnvironmentDirectionalLight& sun,
    EnvironmentSkyLight& skyLight,
    EnvironmentHeightFog& fog,
    EnvironmentSkyAtmosphere& atmosphere,
    const we::math::Vec3& cameraPosition) {
    sun.Color = sun.GetColorFromTemperature();

    const we::math::Vec3 worldOrigin = GetWorldOrigin(cameraPosition);
    const float altitudeMeters = std::max(cameraPosition.y - worldOrigin.y, 0.0f);
    atmosphere.EyeAltitude = std::max(std::round(altitudeMeters * 0.1f) * 0.01f, 0.001f);

    if (skyLight.RealTimeCapture) {
        skyLight.UpperHemisphereColor = ComputeSkyLightUpper(sun, atmosphere);
        skyLight.LowerHemisphereColor = ComputeSkyLightLower(sun, fog, atmosphere);
    }

    fog.FogColor = ComputeFogColor(sun, atmosphere);
}

} // namespace we::runtime::world::environment
