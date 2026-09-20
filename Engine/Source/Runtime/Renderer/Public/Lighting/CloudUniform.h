// ==============================================================================
// WindEffects — Renderer — CloudUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>

namespace we::runtime::renderer {

// Must match CloudBuffer in VolumetricClouds.hlsl (std140 float packing).
// HZD SIGGRAPH 2015 density path only — Texture1/Texture2/Curl + weather/type.
struct CloudUniform {
    float planetRadius = 6371000.0f; // meters
    // Cloud layer AGL: ~1.5 km base → ~11 km top (strato → CB).
    float innerOffset = 1500.0f;
    float outerOffset = 11000.0f;
    // Multiplier on WeatherMap.R (coverage amount).
    float coverage = 0.95f;

    // Type fallback when weather B is weak: 0=stratus .. 1=CB.
    float cloudType = 0.45f;
    float precipitation = 0.0f;
    float timeSeconds = 0.0f;
    float densityScale = 0.48f;

    float windDirX = 1.0f;
    float windDirY = 0.0f;
    float windDirZ = 0.12f;
    float windSpeed = 28.0f;

    // Texture1 fundamental reusable noise period (1/meters) — NOT cloud diameter.
    float baseNoiseScale = 1.0f / 4096.0f;
    // Texture2 erosion-noise sample scale.
    float detailNoiseScale = 0.0035f;
    float curlStrength = 0.95f;
    float absorption = 0.11f;

    float ambient = 0.48f;
    float silverIntensity = 0.75f;
    float silverSpread = 2.6f;
    float powderStrength = 0.48f;

    float phaseGForward = 0.65f;
    float phaseGBack = -0.18f;
    uint32_t maxSteps = 192;
    uint32_t lightSteps = 5;

    float enabled = 1.0f;
    float lightMarchLength = 1000.0f;
    float albedo = 0.94f;
    // Weather map: 1024 texels ≈ 40 km → scale ≈ 1/40000.
    float weatherMapScale = 0.000025f;

    // ---- HZD density controls ----
    float erosionStrength = 0.32f;
    float bottomWispStrength = 0.55f;
    float bottomDensityReduce = 0.70f;
    float typeWeightSharpness = 1.6f;

    // Runtime Remap(Texture1.R, worleyFBM-1) blend. 0 = use baked R directly.
    float baseWorleyInfluence = 0.0f;
    float coverageRemapLow = 0.08f;
    float coverageRemapHigh = 0.72f;
    float precipCbBlend = 0.55f;

    // Formation distribution — controls WHERE clouds appear, HOW LARGE,
    // and HOW MUCH empty sky exists between formations.
    float formationFreqLarge  = 0.000018f;  // ~55 km period — regional cloud banks
    float formationFreqMedium = 0.000055f;  // ~18 km period — individual cloud groups
    float formationFreqSmall  = 0.00016f;   // ~6 km period — scattered small clouds
    float formationThreshold  = 0.42f;      // global coverage control (higher = fewer formations)
};

inline constexpr std::uint32_t kCloudUniformSize = 160;
static_assert(sizeof(CloudUniform) == kCloudUniformSize,
    "CloudUniform packing must match HLSL CloudBuffer.");

} // namespace we::runtime::renderer
