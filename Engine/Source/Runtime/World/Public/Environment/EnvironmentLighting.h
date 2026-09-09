// ==============================================================================
// WindEffects — World — EnvironmentLighting
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"
#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentExposureController.h"
#include "Environment/EnvironmentHeightFog.h"
#include "Environment/EnvironmentSkyAtmosphere.h"
#include "Environment/EnvironmentSkyLight.h"
#include "Environment/EnvironmentVolumetricClouds.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

WORLD_API we::math::Vec3 TemperatureKelvinToRgb(int kelvin);
WORLD_API we::math::Vec3 EulerDegreesToLightDirection(const we::math::Vec3& rotationDegrees);
WORLD_API we::math::Vec3 SunDirectionToSky(const we::math::Vec3& lightTravelDirection);

WORLD_API we::runtime::renderer::SceneEnvironmentUniform BuildSceneEnvironmentUniform(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyLight& skyLight,
    const EnvironmentSkyAtmosphere& atmosphere,
    const EnvironmentHeightFog& fog,
    const EnvironmentVolumetricClouds& clouds,
    const EnvironmentExposureController& exposure,
    const we::math::Vec3& worldOrigin);

} // namespace we::runtime::world::environment
