// ==============================================================================
// WindEffects — World — EnvironmentLighting
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Environment/EnvironmentLighting.h"
#include "Environment/EnvironmentManager.h"
#include "Core/Math/GlmInterop.h"
#include "Lighting/DaylightConfig.h"
#include "Lighting/EnvironmentLightingEvaluator.h"

#include <algorithm>
#include <cmath>
namespace we::runtime::world::environment {

namespace {

float SrgbChannelToLinear(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

we::math::Vec3 SrgbToLinear(const we::math::Vec3& srgb) {
    return we::math::Vec3(
        SrgbChannelToLinear(srgb.x),
        SrgbChannelToLinear(srgb.y),
        SrgbChannelToLinear(srgb.z));
}

} // namespace

we::math::Vec3 TemperatureKelvinToRgb(int kelvin) {
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

    const we::math::Vec3 srgb(
        std::clamp(red / 255.0f, 0.0f, 1.0f),
        std::clamp(green / 255.0f, 0.0f, 1.0f),
        std::clamp(blue / 255.0f, 0.0f, 1.0f));
    return SrgbToLinear(srgb);
}

we::math::Vec3 EulerDegreesToLightDirection(const we::math::Vec3& rotationDegrees) {
    const float pitch = glm::radians(rotationDegrees.x);
    const float yaw = glm::radians(rotationDegrees.y);

    glm::vec3 direction;
    // Rotation pitch follows editor convention: negative pitch = sun above horizon,
    // light travels toward the scene (downward in world +Y-up space).
    direction.x = std::cos(pitch) * std::sin(yaw);
    direction.y = std::sin(pitch);
    direction.z = std::cos(pitch) * std::cos(yaw);
    return we::math::FromGlm(glm::normalize(direction));
}

we::math::Vec3 SunDirectionToSky(const we::math::Vec3& lightTravelDirection) {
    return we::math::FromGlm(glm::normalize(-we::math::ToGlm(lightTravelDirection)));
}

we::runtime::renderer::SceneEnvironmentUniform BuildSceneEnvironmentUniform(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyLight& skyLight,
    const EnvironmentSkyAtmosphere& atmosphere,
    const EnvironmentHeightFog& fog,
    const EnvironmentExposureController& exposure,
    const we::math::Vec3& worldOriginHint) {

    EnvironmentManager manager;
    const float sunDerivedEV = manager.ComputeExposureEV(sun);
    const we::math::Vec3 worldOrigin = manager.GetWorldOrigin(worldOriginHint);

    // DaylightConfig.h — single CPU sun scale (keep in sync with DaylightConfig.hlsli).
    constexpr float kSunArtistToIrradiance = we::runtime::renderer::kDaylightSunArtistToIrradiance;

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
    // Auto-exposure must use the sun-derived EV — ExposureEV stays 0 as a manual override.
    // Using ExposureEV while AutoExposure=true left EV at 0 and blew the sky white.
    uniform.exposureEV = exposure.GetEffectiveExposureEV(sunDerivedEV);
    if (exposure.AutoExposure) {
        uniform.exposureEV = std::clamp(
            sunDerivedEV + exposure.ExposureCompensation,
            exposure.MinEV,
            exposure.MaxEV);
    }
    uniform.planetRadius = 6360.0f;
    uniform.atmosphereHeight = 60.0f;
    uniform.multiScatterStrength = atmosphere.MultiScatterStrength;
    uniform.eyeAltitude = atmosphere.EyeAltitude;
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
    uniform.enableSunDisk = 1.0f;
    uniform.pipelineFixedExposureMultiplier = 0.0f;

    // Derive sky ambient / irradiance from the same Sun+Atmosphere model used by
    // ProceduralSky and the volumetric kernel (not an unrelated painted ambient).
    {
        using we::runtime::renderer::EnvironmentLightingEvaluator;
        const auto envCtx = EnvironmentLightingEvaluator::FromEnvironmentUniform(uniform);
        EnvironmentLightingEvaluator::ApplyToEnvironmentUniform(uniform, envCtx);
        // Preserve artist lower-hemisphere tint as a soft ground bounce mix.
        uniform.skyLightLowerColor = {
            uniform.skyLightLowerColor.x * 0.35f + skyLight.LowerHemisphereColor.x * 0.65f,
            uniform.skyLightLowerColor.y * 0.35f + skyLight.LowerHemisphereColor.y * 0.65f,
            uniform.skyLightLowerColor.z * 0.35f + skyLight.LowerHemisphereColor.z * 0.65f};
    }

    return uniform;
}

} // namespace we::runtime::world::environment
