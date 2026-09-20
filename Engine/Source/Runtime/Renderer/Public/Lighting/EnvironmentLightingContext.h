// ==============================================================================
// WindEffects — Renderer — EnvironmentLightingContext
// Shared atmosphere/environment lighting contract for surfaces + volumetrics.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

#include "Core/Math/Types.h"
#include "Renderer/Export.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

/// Scene-referred environment lighting derived from Sun + Atmosphere.
/// Visible sky radiance and lighting irradiance must share this model.
struct RENDERER_API EnvironmentLightingContext {
    we::math::Vec3 sunDirection{0.3f, -0.8f, 0.2f};
    we::math::Vec3 sunRadiance{1.0f, 0.98f, 0.95f};
    float sunIntensity = 1.2f;
    float sunAngularRadius = 0.004675f;

    we::math::Vec3 skyIrradianceUpper{0.35f, 0.42f, 0.55f};
    we::math::Vec3 skyIrradianceLower{0.15f, 0.16f, 0.18f};
    float skyIntensity = 1.0f;

    we::math::Vec3 skyRadianceZenith{0.18f, 0.46f, 0.88f};
    we::math::Vec3 skyRadianceHorizon{0.52f, 0.70f, 0.90f};

    we::math::Vec3 sunTransmittance{1.0f, 1.0f, 1.0f};

    we::math::Vec3 rayleigh{0.005802f, 0.013558f, 0.033100f};
    float mie = 0.003996f;
    we::math::Vec3 ozone{0.00065f, 0.00188f, 0.000085f};
    float mieAnisotropy = 0.80f;
    float planetRadiusKm = 6360.0f;
    float atmosphereHeightKm = 60.0f;
    float multiScatterStrength = 1.0f;
    float eyeAltitudeKm = 0.001f;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
