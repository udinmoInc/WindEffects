#include "Environment/EnvironmentLighting.h"

#include "Environment/EnvironmentManager.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

namespace {

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

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

    const glm::vec3 srgb(Clamp01(red / 255.0f), Clamp01(green / 255.0f), Clamp01(blue / 255.0f));
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

    we::runtime::renderer::SceneEnvironmentUniform uniform{};
    uniform.sunDirection = sun.GetLightDirection();
    // Artist-facing intensity scaled to approximate outdoor solar illuminance response.
    constexpr float kSunIrradianceScale = 1.35f;
    uniform.sunIntensity = sun.Intensity * kSunIrradianceScale;
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
    uniform.exposureEV = exposure.GetEffectiveExposureEV(sunDerivedEV);
    uniform.planetRadius = 6360.0f;
    uniform.atmosphereHeight = 60.0f;
    uniform.multiScatterStrength = atmosphere.MultiScatterStrength;
    uniform.eyeAltitude = atmosphere.EyeAltitude;
    uniform.cloudCoverage = clouds.Coverage;
    uniform.cloudAltitude = clouds.Altitude;
    uniform.cloudExtinction = clouds.Extinction;
    uniform.enableClouds = clouds.Enabled ? 1.0f : 0.0f;
    uniform.cloudColor = clouds.CloudColor;
    uniform.enableVolumetricFog = fog.VolumetricFog ? 1.0f : 0.0f;
    uniform.exposureCompensation = exposure.ExposureCompensation;
    uniform.sunAngularRadius = 0.004675f;
    uniform.hdrSkyLuminance = manager.ComputeHdrSkyLuminance(sun, atmosphere);
    uniform.sunCastShadows = sun.CastDynamicShadows ? 1 : 0;
    uniform.sunTemperature = sun.TemperatureKelvin;
    uniform.bloomIntensity = 0.15f;
    uniform.bloomThreshold = 4.0f;
    uniform.enableAutoExposure = exposure.AutoExposure ? 1.0f : 0.0f;
    return uniform;
}

} // namespace we::runtime::world::environment
