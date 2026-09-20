// ==============================================================================
// WindEffects — World — EnvironmentSkyAtmosphere
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"

#include <cstdint>
#include <string>
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

class WORLD_API EnvironmentSkyAtmosphere {
public:
    std::uint64_t EntityId = 0;

    std::string SkyMaterial = "Assets/Materials/M_Sky_Default.mat";

    float RayleighScattering = 0.005802f; // Earth R (1/km) ≡ 5.802e-6 m^-1
    float MieScattering = 0.003996f;      // Earth Mie scatter (1/km) ≡ 3.996e-6 m^-1
    float MieAnisotropy = 0.80f;          // Soft solar corona (g ≈ 0.76–0.80)
    float MultiScatterStrength = 1.0f;
    float EyeAltitude = 0.001f;
    we::math::Vec3 OzoneAbsorption{ 0.00065f, 0.0018f, 0.00008f };
    float AerialPerspectiveStartDepth = 0.1f;
    we::math::Vec3 GroundAlbedo{ 0.25f, 0.28f, 0.30f }; // Soft warm ground bounce
    // 0 = off. Atmosphere 1–15 / 101–104.
    // Clouds: 116–118 voxel/raw; 300–311 HZD density + WeatherMap R/G/B/final
    // (Texture1, Perlin-Worley, height, coverage, base, Texture2, erosion,
    //  bottom wisps, curl, final).
    int AtmosphereDebugMode = 0;

    void ApplyDefaults();
    we::math::Vec3 GetRayleighColor() const;
    we::math::Vec3 GetOzoneAbsorption() const;
};

}
