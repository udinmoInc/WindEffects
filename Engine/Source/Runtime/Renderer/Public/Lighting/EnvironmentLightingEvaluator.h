// ==============================================================================
// WindEffects — Renderer — EnvironmentLightingEvaluator
// Derives sky irradiance / radiance / sun transmittance from Sun + Atmosphere.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

#include "Lighting/EnvironmentLightingContext.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Export.h"

namespace we::runtime::renderer {

/// CPU-side evaluation of the shared atmosphere/environment lighting model.
/// Must stay consistent with Shaders/Common/EnvironmentLighting.hlsli.
struct RENDERER_API EnvironmentLightingEvaluator {
    [[nodiscard]] static EnvironmentLightingContext FromEnvironmentUniform(
        const SceneEnvironmentUniform& env);

    static void ApplyToEnvironmentUniform(
        SceneEnvironmentUniform& env,
        const EnvironmentLightingContext& ctx);

    [[nodiscard]] static we::math::Vec3 EvaluateSkyIrradianceUpper(
        const we::math::Vec3& sunTravelDir,
        const we::math::Vec3& sunRadiance,
        float sunIntensity,
        const we::math::Vec3& rayleigh,
        float mie,
        float multiScatter);

    [[nodiscard]] static void EvaluateSkyRadianceColors(
        const we::math::Vec3& sunTravelDir,
        const we::math::Vec3& rayleigh,
        float sunElevationFactor,
        we::math::Vec3& outZenith,
        we::math::Vec3& outHorizon);

    [[nodiscard]] static we::math::Vec3 EvaluateSunTransmittance(
        const we::math::Vec3& sunTravelDir,
        const we::math::Vec3& rayleigh,
        float mie,
        const we::math::Vec3& ozone);
};

} // namespace we::runtime::renderer
