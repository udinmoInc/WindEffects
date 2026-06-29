#ifndef WE_SKY_ATMOSPHERE_HLSLI
#define WE_SKY_ATMOSPHERE_HLSLI

#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"

// Procedural sky atmosphere (editor + runtime). Full LUT path added later.
float3 WE_SampleSkyAtmosphere(float3 worldDir, float3 sunDir, float sunIntensity)
{
    float elevation = worldDir.y * 0.5 + 0.5;
    float3 zenith = float3(0.25, 0.45, 0.85);
    float3 horizon = float3(0.85, 0.75, 0.55);
    float3 sky = lerp(horizon, zenith, pow(saturate(elevation), 1.2));
    float sunDisk = pow(saturate(dot(worldDir, sunDir)), 512.0) * sunIntensity;
    return sky + sunDisk;
}

#endif // WE_SKY_ATMOSPHERE_HLSLI
