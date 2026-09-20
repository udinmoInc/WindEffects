#ifndef WE_DAYLIGHT_CONFIG_HLSLI
#define WE_DAYLIGHT_CONFIG_HLSLI

// Display encode + legacy helpers. Sky radiance lives in EnvironmentLighting.hlsli.

#include "EnvironmentLighting.hlsli"

// --- Display encode (temporary until TonemapPass owns exposure+tonemap) ---
static const float WE_DAYLIGHT_EXPOSURE = 1.25;
static const float WE_DAYLIGHT_SHOULDER = 0.38;

// --- Cloud scattering scales (modulation on shared volumetric lighting) ---
static const float WE_CLOUD_SUN_MULT       = 2.85;
static const float WE_CLOUD_AMBIENT_MULT   = 0.11;
static const float WE_CLOUD_DENSITY_MIN    = 0.48;
static const float WE_CLOUD_DENSITY_MAX    = 0.92;
static const float WE_CLOUD_EXPOSURE       = 1.12;
static const float WE_CLOUD_SHOULDER       = 0.48;

float3 WE_EncodeDaylight(float3 linearColor)
{
    const float3 exposed = max(linearColor, 0.0) * WE_DAYLIGHT_EXPOSURE;
    return saturate(exposed / (1.0 + exposed * WE_DAYLIGHT_SHOULDER));
}

float3 WE_EncodeDaylightCloud(float3 linearColor)
{
    const float3 exposed = max(linearColor, 0.0) * WE_CLOUD_EXPOSURE;
    return saturate(exposed / (1.0 + exposed * WE_CLOUD_SHOULDER));
}

// Legacy entry — sunDir is "to sun"; EnvironmentLighting expects light-travel.
float3 WE_EvalDaylightSky(float3 viewDir, float3 sunDir, float3 sunCol, float sunIntensity,
                          float sunAngularRadius, float enableDisk)
{
    const float3 sunTravel = -normalize(sunDir);
    return WE_EvalSkyRadiance(
        viewDir, sunTravel, sunCol, sunIntensity, sunAngularRadius, enableDisk,
        atmosphereRayleigh, mieScattering, ozoneAbsorption, mieAnisotropy);
}

#endif // WE_DAYLIGHT_CONFIG_HLSLI
