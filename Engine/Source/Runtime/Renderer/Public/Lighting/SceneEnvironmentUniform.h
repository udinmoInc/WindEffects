// ==============================================================================
// WindEffects — Renderer — SceneEnvironmentUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
    we::math::Vec3 atmosphereRayleigh{0.005802f, 0.013558f, 0.033100f}; // Earth 1/km
    float mieScattering = 0.003996f;
    we::math::Vec3 ozoneAbsorption{0.00065f, 0.00188f, 0.000085f};
    float mieAnisotropy = 0.80f;
    we::math::Vec3 worldOrigin{0.0f, 0.0f, 0.0f};
    float exposureEV = 0.0f;
    float planetRadius = 6360.0f;
    float atmosphereHeight = 60.0f;
    float multiScatterStrength = 1.0f;
    float eyeAltitude = 0.0f;
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
    float enableSunDisk = 1.0f;
    float pipelineFixedExposureMultiplier = 0.0f;
};

constexpr std::size_t kSceneEnvironmentUniformSize = 196;
static_assert(sizeof(SceneEnvironmentUniform) == kSceneEnvironmentUniformSize,
    "Environment UBO size drift — rebuild ALL Renderer/World translation units that include this header.");
static_assert(offsetof(SceneEnvironmentUniform, enableVolumetricFog) == 144,
    "Environment UBO packing drift (enableVolumetricFog).");
static_assert(offsetof(SceneEnvironmentUniform, exposureCompensation) == 148,
    "Environment UBO packing drift (exposureCompensation).");
static_assert(offsetof(SceneEnvironmentUniform, atmosphereDebugMode) == 180,
    "Environment UBO packing drift (atmosphereDebugMode).");
static_assert(offsetof(SceneEnvironmentUniform, enableSunDisk) == 188,
    "Environment UBO packing drift (enableSunDisk).");
static_assert(offsetof(SceneEnvironmentUniform, pipelineFixedExposureMultiplier) == 192,
    "Environment UBO packing drift (pipelineFixedExposureMultiplier).");

} // namespace we::runtime::renderer

#pragma warning(pop)
