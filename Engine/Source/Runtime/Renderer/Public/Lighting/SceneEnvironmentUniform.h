#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Renderer/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>
#include <cstddef>

namespace we::runtime::renderer {

// Must match EnvironmentBuffer.hlsli (std140-compatible packing).
struct RENDERER_API SceneEnvironmentUniform {
    we::math::Vec3 sunDirection{0.3f, -0.8f, 0.2f};
    float sunIntensity = 1.2f;
    we::math::Vec3 sunColor{1.0f, 0.98f, 0.95f};
    float skyLightIntensity = 1.0f;
    we::math::Vec3 skyAmbientColor{0.35f, 0.42f, 0.55f};
    float fogDensity = 0.0f;
    we::math::Vec3 skyLightLowerColor{0.15f, 0.16f, 0.18f};
    float fogHeightFalloff = 0.2f;
    we::math::Vec3 fogColor{0.7f, 0.75f, 0.85f};
    float fogStartDistance = 0.0f;
    we::math::Vec3 atmosphereRayleigh{0.18f, 0.42f, 0.82f};
    float mieScattering = 0.004f;
    we::math::Vec3 ozoneAbsorption{0.00065f, 0.00188f, 0.000085f};
    float mieAnisotropy = 0.76f;
    we::math::Vec3 worldOrigin{0.0f, 0.0f, 0.0f};
    float exposureEV = 0.0f;
    float planetRadius = 6360.0f;
    float atmosphereHeight = 60.0f;
    float multiScatterStrength = 1.0f;
    float eyeAltitude = 0.0f;
    float cloudCoverage = 0.55f;
    float cloudAltitude = 1250.0f;
    float cloudExtinction = 0.35f;
    float enableClouds = 1.0f;
    we::math::Vec3 cloudColor{0.95f, 0.96f, 0.98f};
    float enableVolumetricFog = 0.0f;
    float exposureCompensation = 0.0f;
    float sunAngularRadius = 0.004675f;
    float hdrSkyLuminance = 1.0f;
    int sunCastShadows = 1;
    int sunTemperature = 5500;
    float bloomIntensity = 0.15f;
    float bloomThreshold = 4.0f;
    float enableAutoExposure = 0.0f;
    int atmosphereDebugMode = 0;
    int pipelineBypassToneMapping = 0;
    float cloudTemporalBlend = 0.0f;
    int cloudHistoryValid = 0;
    float enableSunDisk = 1.0f;
    float pipelineFixedExposureMultiplier = 0.0f;

    float cloudDensityMult = 1.0f;
    float cloudThickness = 800.0f;
    float cloudBottomAltitude = 900.0f;
    float cloudTopAltitude = 1600.0f;
    float cloudBoundsPad0 = 0.0f;
    float cloudBoundsPad1 = 0.0f;
    we::math::Vec3 cloudWindDir{1.0f, 0.0f, 0.25f};
    float cloudWindSpeed = 18.0f;
    float cloudNoiseScale = 1.0f;
    float cloudDetailScale = 3.7f;
    float cloudLightingIntensity = 1.0f;
    float cloudSilverLining = 0.85f;
    float cloudAmbient = 0.42f;
    float cloudMultiScatter = 0.35f;
    float cloudPhaseG = 0.6f;
    float cloudPowder = 0.5f;
    float cloudSeed = 17.3f;
    float cloudAnimTime = 0.0f;
    float cloudShadowStrength = 0.65f;
    int cloudQualitySteps = 32;
    float cloudShapeNoise = 0.8f;
    float cloudErosionNoise = 0.28f;
};

constexpr std::size_t kSceneEnvironmentUniformSize = 328;
static_assert(sizeof(SceneEnvironmentUniform) == kSceneEnvironmentUniformSize,
    "Environment UBO size drift — rebuild ALL Renderer/World translation units that include this header.");
static_assert(offsetof(SceneEnvironmentUniform, enableClouds) == 156, "Environment UBO packing drift (enableClouds).");
static_assert(offsetof(SceneEnvironmentUniform, cloudDensityMult) == 232, "Environment UBO packing drift (cloudDensityMult).");
static_assert(offsetof(SceneEnvironmentUniform, cloudWindDir) == 256, "Environment UBO packing drift (cloudWindDir pad).");
static_assert(offsetof(SceneEnvironmentUniform, cloudQualitySteps) == 316, "Environment UBO packing drift (cloudQualitySteps).");
static_assert(offsetof(SceneEnvironmentUniform, cloudShapeNoise) == 320, "Environment UBO packing drift (cloudShapeNoise).");
static_assert(offsetof(SceneEnvironmentUniform, cloudErosionNoise) == 324, "Environment UBO packing drift (cloudErosionNoise).");

} // namespace we::runtime::renderer

#pragma warning(pop)
