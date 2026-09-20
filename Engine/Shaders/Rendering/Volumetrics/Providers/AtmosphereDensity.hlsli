#ifndef WE_ATMOSPHERE_DENSITY_PROVIDER_HLSLI
#define WE_ATMOSPHERE_DENSITY_PROVIDER_HLSLI

// Atmosphere as a volumetric density source (Rayleigh + Mie air).
// Does NOT own lighting — the unified kernel evaluates light transport.

#include "../VolumeSample.hlsli"
#include "../../AtmosphereCommon.hlsli"

VolumeSample EvaluateAtmosphereVolumeSample(
    float3 worldPos,
    float3 worldOrigin,
    float planetRadiusKm,
    float atmosphereHeightKm,
    float3 rayleighCoeff,
    float mieCoeff,
    float enabled)
{
    if (enabled < 0.5)
        return EmptyVolumeSample();

    // Convert engine meters → atmosphere km frame (planet center below origin).
    const float planetRadiusM = max(planetRadiusKm, 1.0) * 1000.0;
    const float3 planetPos = (worldPos - worldOrigin) + float3(0.0, planetRadiusM, 0.0);
    const float heightM = max(length(planetPos) - planetRadiusM, 0.0);
    const float heightKm = heightM * 0.001;
    const float atmoTopKm = max(atmosphereHeightKm, 1.0);
    if (heightKm > atmoTopKm)
        return EmptyVolumeSample();

    const float rd = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
    const float md = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);

    // Gentle aerial-perspective density — must NOT opaque-wipe ProceduralSky.
    const float rayleighAvg = (rayleighCoeff.x + rayleighCoeff.y + rayleighCoeff.z) * (1.0 / 3.0);
    const float density = (rayleighAvg * rd + mieCoeff * md) * 0.35;
    if (density <= 1e-8)
        return EmptyVolumeSample();

    const float extinction = density;
    const float scattering = density * 0.95;
    // Mie anisotropy as media anisotropy cue.
    return MakeVolumeSample(density, extinction, scattering, 0.0);
}

#endif // WE_ATMOSPHERE_DENSITY_PROVIDER_HLSLI
