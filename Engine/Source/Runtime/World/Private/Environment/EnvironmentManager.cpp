// ==============================================================================
// WindEffects — World — EnvironmentManager
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Environment/EnvironmentManager.h"

#include "Environment/EnvironmentLighting.h"
#include "Core/Math/GlmInterop.h"
#include "Lighting/DaylightConfig.h"

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
    const glm::vec3 sunColor = we::math::ToGlm(sun.GetColorFromTemperature());
    (void)atmosphere;

    // DaylightConfig — sky ambient fill for meshes/clouds.
    const glm::vec3 skyBlue = glm::vec3(0.22f, 0.48f, 0.88f);
    const float elevW = 0.45f + elevation * 0.55f;
    const float sunI = std::min(sun.Intensity, 10.0f);
    const glm::vec3 scattered = skyBlue * elevW * sunI
        * we::runtime::renderer::kDaylightSkyLightUpperScale;
    const glm::vec3 sunBounce = sunColor * sunI * elevation
        * we::runtime::renderer::kDaylightSkyLightSunBounce;
    const float night = Clamp01(0.15f - elevation);
    const glm::vec3 nightGlow = glm::vec3(0.03f, 0.05f, 0.10f) * night;
    return we::math::FromGlm(scattered + sunBounce + nightGlow);
}

we::math::Vec3 EnvironmentManager::ComputeSkyLightLower(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentHeightFog& fog,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const float night = Clamp01(1.0f - elevation * 2.5f);
    const glm::vec3 sunColor = we::math::ToGlm(sun.GetColorFromTemperature());
    const glm::vec3 ground = we::math::ToGlm(atmosphere.GroundAlbedo);
    const glm::vec3 horizon = glm::vec3(0.28f, 0.42f, 0.60f) * (0.30f + elevation * 0.30f);
    const glm::vec3 fogTint = we::math::ToGlm(fog.FogColor) * 0.12f;
    const glm::vec3 dayLower = horizon + ground * 0.20f + sunColor * 0.025f * elevation + fogTint;
    return we::math::FromGlm(glm::mix(dayLower * 0.12f, dayLower, 1.0f - night * 0.85f));
}

we::math::Vec3 EnvironmentManager::ComputeFogColor(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyAtmosphere& atmosphere) const {
    const glm::vec3 sunDir = we::math::ToGlm(SunDirectionToSky(sun.GetLightDirection()));
    const float elevation = Clamp01(sunDir.y);
    const glm::vec3 sunColor = we::math::ToGlm(sun.GetColorFromTemperature());
    const glm::vec3 rayleigh = we::math::ToGlm(atmosphere.GetRayleighColor());
    const glm::vec3 skyNorm = glm::normalize(rayleigh + glm::vec3(1e-5f));

    const glm::vec3 dayFog = skyNorm * glm::vec3(0.55f, 0.68f, 0.88f) + sunColor * 0.12f;
    const glm::vec3 sunsetFog = sunColor * 0.55f + skyNorm * glm::vec3(0.45f, 0.40f, 0.55f);
    const glm::vec3 nightFog = glm::vec3(0.08f, 0.10f, 0.16f);

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
