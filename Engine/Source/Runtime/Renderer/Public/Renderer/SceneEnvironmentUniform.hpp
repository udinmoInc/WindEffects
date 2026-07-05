#pragma once

#if WE_HAS_GLM
#include <glm/glm.hpp>
#else
#ifndef GLM_FALLBACK_TYPES_DEFINED
#define GLM_FALLBACK_TYPES_DEFINED
struct glm_vec3 { float x, y, z; };
struct glm_vec4 { float x, y, z, w; };
struct glm_ivec2 { int x, y; };
namespace glm {
    using vec3 = glm_vec3;
    using vec4 = glm_vec4;
    using ivec2 = glm_ivec2;
}
#endif
#endif

namespace we::runtime::renderer {

struct SceneEnvironmentUniform {
    glm::vec3 sunDirection{ 0.38f, 0.92f, 0.18f };
    float sunIntensity = 10.0f;
    glm::vec3 sunColor{ 1.0f, 0.96f, 0.86f };
    float skyLightIntensity = 1.0f;
    glm::vec3 skyAmbientColor{ 0.05f, 0.08f, 0.12f };
    float fogDensity = 0.02f;
    glm::vec3 skyLightLowerColor{ 0.05f, 0.05f, 0.06f };
    float fogHeightFalloff = 0.2f;
    glm::vec3 fogColor{ 0.72f, 0.78f, 0.85f };
    float fogStartDistance = 0.0f;
    glm::vec3 atmosphereRayleigh{ 0.005802f, 0.013558f, 0.033100f };
    float mieScattering = 0.003996f;
    glm::vec3 ozoneAbsorption{ 0.00065f, 0.0018f, 0.00008f };
    float mieAnisotropy = 0.76f;
    glm::vec3 worldOrigin{ 0.0f, 0.0f, 0.0f };
    float exposureEV = 2.35f;
    float planetRadius = 6360.0f;
    float atmosphereHeight = 60.0f;
    float multiScatterStrength = 1.0f;
    float eyeAltitude = 0.001f;
    float cloudCoverage = 0.45f;
    float cloudAltitude = 5000.0f;
    float cloudExtinction = 0.35f;
    float enableClouds = 1.0f;
    glm::vec3 cloudColor{ 0.95f, 0.96f, 0.98f };
    float enableVolumetricFog = 1.0f;
    float exposureCompensation = 0.0f;
    float sunAngularRadius = 0.004675f;
    float hdrSkyLuminance = 1.0f;
    int sunCastShadows = 1;
    int sunTemperature = 6500;
    float bloomIntensity = 0.65f;
    float bloomThreshold = 0.85f;
    float enableAutoExposure = 1.0f;
    float postPadding = 0.0f;
};

} // namespace we::runtime::renderer
