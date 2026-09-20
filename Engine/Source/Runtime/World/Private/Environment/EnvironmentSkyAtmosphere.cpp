// ==============================================================================
// WindEffects — World — EnvironmentSkyAtmosphere
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Environment/EnvironmentSkyAtmosphere.h"

#include "Core/Math/GlmInterop.h"
namespace we::runtime::world::environment {

void EnvironmentSkyAtmosphere::ApplyDefaults() {
    // Earth daylight: Rayleigh/Mie in 1/km (= physical m^-1 × 1000).
    // R: 5.802e-6, G: 13.558e-6, B: 33.100e-6 m^-1
    // Mie scatter: 3.996e-6, Mie absorb: 4.440e-6 m^-1
    // Scale heights: Rayleigh ~8 km, Mie ~1.2 km (see AtmosphereCommon.hlsli).
    RayleighScattering = 0.005802f;
    MieScattering = 0.003996f;
    MieAnisotropy = 0.80f;
    MultiScatterStrength = 1.0f;
    EyeAltitude = 0.001f;
    OzoneAbsorption = we::math::Vec3(0.00065f, 0.0018f, 0.00008f);
    AerialPerspectiveStartDepth = 0.1f;
    GroundAlbedo = we::math::Vec3(0.25f, 0.28f, 0.30f);
}

we::math::Vec3 EnvironmentSkyAtmosphere::GetRayleighColor() const {
    // Sea-level Rayleigh scattering coefficients (1/km) — Earth daylight.
    // Equivalent to float3(5.802e-6, 13.558e-6, 33.100e-6) per meter.
    constexpr float kRed = 0.005802f;
    constexpr float kGreen = 0.013558f;
    constexpr float kBlue = 0.033100f;
    const float scale = RayleighScattering / kRed;
    return we::math::Vec3(kRed * scale, kGreen * scale, kBlue * scale);
}

we::math::Vec3 EnvironmentSkyAtmosphere::GetOzoneAbsorption() const {
    return OzoneAbsorption;
}

} // namespace we::runtime::world::environment
