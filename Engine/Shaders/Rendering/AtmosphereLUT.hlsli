#ifndef WE_ATMOSPHERE_LUT_HLSLI
#define WE_ATMOSPHERE_LUT_HLSLI

#include "AtmosphereIntegrator.hlsli"
#include "SkyAtmosphere.hlsli"
#include "../Common/Color.hlsli"

// Transmittance LUT: UE5 distance/height parameterization.
float2 WE_TransmittanceLUTCoord(float heightKm, float cosZenith, WE_AtmosphereParams params)
{
    const float viewHeightKm = params.planetRadius + max(heightKm, 0.0);
    return WE_LutTransmittanceParamsToUv(viewHeightKm, cosZenith, params);
}

float3 WE_SampleTransmittanceLUT(
    Texture2D lut,
    SamplerState samp,
    float heightKm,
    float cosZenith,
    WE_AtmosphereParams params)
{
    const float2 uv = WE_TransmittanceLUTCoord(heightKm, cosZenith, params);
    return lut.SampleLevel(samp, uv, 0.0).rgb;
}

float3 WE_SampleMultiScatteringLUT(
    Texture2D lut,
    SamplerState samp,
    float cosSunZenith,
    float heightKm,
    WE_AtmosphereParams params)
{
    const float hNorm = saturate(heightKm / max(params.atmosphereRadius - params.planetRadius, 1.0));
    const float2 uv = float2(cosSunZenith * 0.5 + 0.5, hNorm);
    return lut.SampleLevel(samp, uv, 0.0).rgb;
}

// SkyView LUT: sun-relative azimuth (U) and normalized zenith angle (V).
float3 WE_SampleSkyViewLUT(
    Texture2D lut,
    SamplerState samp,
    float3 viewDir,
    float3 sunDir)
{
    float viewZenithAngle = 0.0;
    const float2 uv = WE_SkyViewLUTCoord(viewDir, sunDir, viewZenithAngle);
    float3 sample = lut.SampleLevel(samp, uv, 0.0).rgb;
    sample = WE_SanitizeHdrColor(sample);
    return max(sample, 0.0);
}

float3 WE_SampleAerialPerspectiveLUT(
    Texture2D lut,
    SamplerState samp,
    float distanceKm,
    float heightKm,
    WE_AtmosphereParams params)
{
    const float dNorm = saturate(distanceKm / 64.0);
    const float hNorm = saturate(heightKm / max(params.atmosphereRadius - params.planetRadius, 1.0));
    return lut.SampleLevel(samp, float2(dNorm, hNorm), 0.0).rgb;
}

// sunColor in EnvironmentBuffer is already linear (from blackbody temperature).
float3 WE_SampleSkyAtmosphereLUT(
    float3 viewDir,
    float3 lightTravelDir,
    float3 cameraPos,
    float3 worldOrigin,
    float3 sunColor,
    float sunIntensity,
    float3 rayleighCoeff,
    float mieCoeff,
    float3 ozoneCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float multiScatterStrength,
    float eyeAltitude,
    float sunAngularRadius,
    Texture2D transmittanceLUT,
    Texture2D multiScatterLUT,
    Texture2D skyViewLUT,
    Texture2D aerialLUT,
    SamplerState lutSampler)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunLinear, sunIntensity, sunAngularRadius);

    const float3 origin = WE_GetAtmosphereOrigin(cameraPos, worldOrigin, params.planetRadius, params.eyeAltitude);

    // Primary sky radiance from baked SkyView LUT (sun-relative UV, includes inscattering).
    float3 sky = WE_SampleSkyViewLUT(skyViewLUT, lutSampler, viewDir, sunDir);

    // Sun disk is rendered at runtime (not baked into SkyView LUT).
    if (enableSunDisk >= 0.5)
        sky += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, sunLinear, params.sunAngularRadius);
    sky = WE_SanitizeHdrColor(sky);
    return max(sky, 0.0);
}

float3 WE_SampleAerialPerspective(
    float3 viewDir,
    float3 lightTravelDir,
    float3 cameraPos,
    float3 worldOrigin,
    float3 surfacePos,
    float3 sunColor,
    float sunIntensity,
    float3 rayleighCoeff,
    float mieCoeff,
    float3 ozoneCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float multiScatterStrength,
    float eyeAltitude,
    float sunAngularRadius,
    Texture2D aerialLUT,
    Texture2D skyViewLUT,
    SamplerState lutSampler)
{
    (void)aerialLUT;
    (void)skyViewLUT;
    (void)lutSampler;

    const float3 relCam = cameraPos - worldOrigin;
    const float3 relSurf = surfacePos - worldOrigin;
    const float dist = length(relCam - relSurf);
    const float3 marchDir = normalize(relSurf - relCam);

    // Analytic inscattering along camera-to-surface segment (view- and sun-dependent).
    const float3 inscatter = WE_SampleSkyAtmosphere(
        marchDir, lightTravelDir, cameraPos, worldOrigin,
        sunColor, sunIntensity * 0.65,
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunAngularRadius);

    const float distFactor = 1.0 - exp(-dist * 0.0015);
    return inscatter * saturate(distFactor);
}

#endif // WE_ATMOSPHERE_LUT_HLSLI
