#ifndef WE_SKY_ATMOSPHERE_HLSLI
#define WE_SKY_ATMOSPHERE_HLSLI

#include "../Common/Color.hlsli"
#include "AtmosphereIntegrator.hlsli"

// sunColor in EnvironmentBuffer is already linear (from blackbody temperature).
float3 WE_SampleSkyAtmosphere(
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
    float sunAngularRadius)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunLinear, sunIntensity, max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));

    const float3 origin = WE_GetAtmosphereOrigin(cameraPos, worldOrigin, params.planetRadius);
    float3 transmittance;
    float3 sky = WE_IntegrateInscattering(viewDir, sunDir, origin, params, transmittance);
    sky += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, sunLinear, params.sunAngularRadius);
    return max(sky, 0.0);
}

float3 WE_SampleSkyLightHemisphere(
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
    bool upperHemisphere)
{
    const float3 basis = upperHemisphere ? float3(0.0, 1.0, 0.0) : float3(0.0, -1.0, 0.0);
    float3 sum = float3(0.0, 0.0, 0.0);
    const int samples = 4;
    [unroll]
    for (int i = 0; i < samples; ++i)
    {
        const float angle = (float(i) / float(samples)) * WE_PI * 2.0;
        const float3 offset = float3(cos(angle), 0.0, sin(angle)) * 0.35;
        const float3 dir = normalize(basis + offset);
        sum += WE_SampleSkyAtmosphere(
            dir, lightTravelDir, cameraPos, worldOrigin,
            sunColor, sunIntensity * 0.25,
            rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
            planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
            sunAngularRadius);
    }
    return sum / float(samples);
}

#endif // WE_SKY_ATMOSPHERE_HLSLI
