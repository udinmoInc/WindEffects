// ==============================================================================
// WindEffects — Renderer — DaylightConfig
// Partly cloudy with guaranteed visible mass (no empty sky from stacked gates).
// ==============================================================================
#pragma once

#include "Lighting/CloudUniform.h"

namespace we::runtime::renderer {

inline constexpr float kDaylightSunArtistToIrradiance = 0.16f;
inline constexpr float kDaylightSkyLightUpperScale = 0.04f;
inline constexpr float kDaylightSkyLightSunBounce = 0.02f;

inline void ApplyDaylightCloudDefaults(CloudUniform& c)
{
    // Opt-in: procedural sky must render correctly with clouds disabled.
    // Enable from cloud tooling / scalability when validating volumetrics.
    c.enabled = 0.0f;
    c.densityScale = 0.74f;
    // Mildly partly cloudy — enough coverage to always show formations.
    c.coverage = 0.70f;
    c.innerOffset = 1850.0f;
    c.outerOffset = 7200.0f;
    c.cloudType = 0.55f;
    c.precipitation = 0.0f;

    c.baseNoiseScale = 1.0f / 13000.0f;
    c.detailNoiseScale = 0.00080f;
    c.weatherMapScale = 0.0000085f;
    c.baseWorleyInfluence = 0.0f;
    // Soft remap — hard floors emptied the sky twice.
    c.coverageRemapLow = 0.20f;
    c.coverageRemapHigh = 0.80f;
    c.precipCbBlend = 0.08f;
    c.typeWeightSharpness = 1.20f;
    c.bottomDensityReduce = 0.28f;

    c.erosionStrength = 0.0f;
    c.bottomWispStrength = 0.0f;
    c.curlStrength = 0.0f;

    c.ambient = 0.11f;
    c.silverIntensity = 0.78f;
    c.silverSpread = 2.0f;
    c.powderStrength = 0.58f;
    c.absorption = 0.16f;
    c.albedo = 0.90f;
    c.phaseGForward = 0.74f;
    c.phaseGBack = -0.18f;
    c.lightMarchLength = 2800.0f;
    c.maxSteps = 256;

    // Formation distribution — visible separated formations + open sky.
    c.formationFreqLarge  = 0.000018f;   // ~55 km regional banks
    c.formationFreqMedium = 0.000055f;   // ~18 km individual groups
    c.formationFreqSmall  = 0.00016f;    // ~6 km scattered clouds
    c.formationThreshold  = 0.42f;       // moderate → visible formations + empty sky
}

} // namespace we::runtime::renderer
