#ifndef WE_CLOUD_DENSITY_PROVIDER_HLSLI
#define WE_CLOUD_DENSITY_PROVIDER_HLSLI

// Cloud volume provider — wraps existing procedural/HZD density into VolumeSample.
// Does NOT redesign cloud morphology; SampleCloudDensity remains the source of truth.

#include "../VolumeSample.hlsli"
#include "../../VolumetricClouds.hlsli"

VolumeSample EvaluateCloudVolumeSample(
    float3 planetPos,
    float innerRadius,
    float outerRadius,
    float coverage,
    float cloudType,
    float precipitation,
    float3 windOffset,
    float baseScale,
    float detailScale,
    float curlStrength,
    float densityScale,
    float absorption,
    float albedo,
    float phaseG,
    bool expensive,
    Texture3D<float4> baseShape,
    Texture3D<float4> detailShape,
    Texture2D<float4> curlNoise,
    SamplerState samp)
{
    CloudDensityResult raw = SampleCloudDensity(
        planetPos, innerRadius, outerRadius,
        coverage, cloudType, precipitation,
        windOffset, baseScale, detailScale, curlStrength,
        expensive,
        baseShape, detailShape, curlNoise, samp);

    const float density = raw.density * densityScale;
    if (density <= 1e-5)
        return EmptyVolumeSample();

    // Extinction from absorption; scattering scaled by albedo.
    const float extinction = density * max(absorption, 1e-4);
    const float scattering = extinction * saturate(albedo);
    VolumeSample s = MakeVolumeSample(density, extinction, scattering, phaseG);
    return s;
}

#endif // WE_CLOUD_DENSITY_PROVIDER_HLSLI
