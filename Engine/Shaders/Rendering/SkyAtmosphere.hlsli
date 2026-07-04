#ifndef WE_SKY_ATMOSPHERE_HLSLI
#define WE_SKY_ATMOSPHERE_HLSLI

#include "../Common/Color.hlsli"
#include "AtmosphereIntegrator.hlsli"

// Higher step count for the procedural (non-LUT) sky path.
static const int WE_PROCEDURAL_ATMOSPHERE_STEPS = 24;

float3 WE_ComputeAtmosphereScattering(
    float3 viewDir,
    float3 sunDir,
    float3 cameraPos,
    float3 worldOrigin,
    WE_AtmosphereParams params)
{
    viewDir = normalize(viewDir);
    sunDir = normalize(sunDir);

    const float3 planetCenter = WE_GetPlanetCenter(cameraPos, worldOrigin, params.planetRadius);
    const float3 rayOrigin = (cameraPos - worldOrigin) - planetCenter;
    const float3 origin = rayOrigin + normalize(rayOrigin + float3(0.0, params.eyeAltitude + 0.001, 0.0)) * 0.001;

    float t0 = 0.0;
    float t1 = 0.0;
    if (!WE_IntersectSphere(origin, viewDir, params.atmosphereRadius, t0, t1))
        return float3(0.0, 0.0, 0.0);

    const float start = max(t0, 0.0);
    const float end = t1;
    const float stepSize = (end - start) / float(WE_PROCEDURAL_ATMOSPHERE_STEPS);

    float3 sumRayleigh = float3(0.0, 0.0, 0.0);
    float3 sumMie = float3(0.0, 0.0, 0.0);
    float3 opticalDepthRayleigh = float3(0.0, 0.0, 0.0);
    float3 opticalDepthMie = float3(0.0, 0.0, 0.0);
    float3 opticalDepthOzone = float3(0.0, 0.0, 0.0);

    const float cosTheta = dot(viewDir, sunDir);
    const float phaseRayleigh = WE_RayleighPhase(cosTheta);
    const float phaseMie = WE_MiePhase(cosTheta, params.mieAnisotropy);

    [loop]
    for (int i = 0; i < WE_PROCEDURAL_ATMOSPHERE_STEPS; ++i)
    {
        const float t = start + (float(i) + 0.5) * stepSize;
        const float3 samplePos = origin + viewDir * t;
        const float heightKm = max(length(samplePos) - params.planetRadius, 0.0);

        const float rayleighDensity = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
        const float mieDensity = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);
        const float ozoneDensity = WE_OzoneDensity(heightKm);

        opticalDepthRayleigh += params.rayleighCoeff * rayleighDensity * stepSize;
        opticalDepthMie += params.mieCoeff * mieDensity * stepSize;
        opticalDepthOzone += params.ozoneCoeff * ozoneDensity * stepSize;

        const float3 sunTransmittance = WE_IntegrateSunTransmittance(samplePos, sunDir, params);
        const float3 tau = opticalDepthRayleigh + opticalDepthMie + opticalDepthOzone;
        const float3 attenuation = exp(-tau) * sunTransmittance;

        sumRayleigh += attenuation * rayleighDensity * stepSize;
        sumMie += attenuation * mieDensity * stepSize;
    }

    const float3 opticalDepthTotal = opticalDepthRayleigh + opticalDepthMie;
    const float3 multiScatter = (1.0 - exp(-opticalDepthTotal * 0.15)) * params.rayleighCoeff
        * params.multiScatterStrength;

    const float3 sunRadiance = params.sunColor * params.sunIntensity;
    return sunRadiance * (
        sumRayleigh * phaseRayleigh * params.rayleighCoeff
        + sumMie * phaseMie * params.mieCoeff
        + multiScatter);
}

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
    float eyeAltitude)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));
    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunLinear, sunIntensity, WE_SUN_ANGULAR_RADIUS);

    const float3 planetCenter = WE_GetPlanetCenter(cameraPos, worldOrigin, params.planetRadius);
    const float3 origin = (cameraPos - worldOrigin) - planetCenter;
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
            planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude);
    }
    return sum / float(samples);
}

#endif // WE_SKY_ATMOSPHERE_HLSLI
