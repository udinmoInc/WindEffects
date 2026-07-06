#ifndef WE_ATMOSPHERE_INTEGRATOR_HLSLI
#define WE_ATMOSPHERE_INTEGRATOR_HLSLI

#include "AtmosphereCommon.hlsli"

float3 WE_IntegrateOpticalDepth(
    float3 origin,
    float3 dir,
    float maxDist,
    WE_AtmosphereParams params,
    out float3 rayleighDepth,
    out float3 mieDepth,
    out float3 ozoneDepth)
{
    rayleighDepth = float3(0.0, 0.0, 0.0);
    mieDepth = float3(0.0, 0.0, 0.0);
    ozoneDepth = float3(0.0, 0.0, 0.0);

    const int steps = WE_SUN_TRANSMITTANCE_STEPS;
    const float stepSize = maxDist / float(steps);
    float3 marchPos = origin;

    [loop]
    for (int i = 0; i < steps; ++i)
    {
        const float heightKm = max(length(marchPos) - params.planetRadius, 0.0);
        const float rd = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
        const float md = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);
        const float od = WE_OzoneDensity(heightKm);

        rayleighDepth += params.rayleighCoeff * rd * stepSize;
        mieDepth += params.mieCoeff * md * stepSize;
        ozoneDepth += params.ozoneCoeff * od * stepSize;
        marchPos += dir * stepSize;
    }

    return exp(-(rayleighDepth + mieDepth + ozoneDepth));
}

float3 WE_IntegrateSunTransmittance(float3 samplePosRel, float3 sunDir, WE_AtmosphereParams params)
{
    float3 rd, md, od;
    const float dist = WE_AtmosphereShellExitDistance(samplePosRel, sunDir, params.atmosphereRadius);
    return WE_IntegrateOpticalDepth(samplePosRel, sunDir, max(dist, 0.001), params, rd, md, od);
}

float3 WE_IntegrateSunTransmittanceLUT(
    float3 samplePosRel,
    float3 sunDir,
    WE_AtmosphereParams params,
    Texture2D transmittanceLUT,
    SamplerState lutSampler)
{
    const float viewHeightKm = max(length(samplePosRel), params.planetRadius);
    const float3 upVector = samplePosRel / max(length(samplePosRel), 1e-4);
    const float viewZenithCosAngle = dot(normalize(sunDir), upVector);
    const float2 uv = WE_LutTransmittanceParamsToUv(viewHeightKm, viewZenithCosAngle, params);
    return transmittanceLUT.SampleLevel(lutSampler, uv, 0.0).rgb;
}

struct WE_InscatteringResult
{
    float3 skyRadiance;
    float3 transmittanceToCamera;
    float3 rayleighContribution;
    float3 mieContribution;
    float3 multiScatterContribution;
    float3 sunDiskContribution;
    float3 opticalDepth;
    float  rayDistanceKm;
};

WE_InscatteringResult WE_IntegrateInscatteringDetailed(
    float3 viewDir,
    float3 sunDir,
    float3 origin,
    WE_AtmosphereParams params)
{
    WE_InscatteringResult result;
    result.skyRadiance = float3(0.0, 0.0, 0.0);
    result.transmittanceToCamera = float3(1.0, 1.0, 1.0);
    result.rayleighContribution = float3(0.0, 0.0, 0.0);
    result.mieContribution = float3(0.0, 0.0, 0.0);
    result.multiScatterContribution = float3(0.0, 0.0, 0.0);
    result.sunDiskContribution = float3(0.0, 0.0, 0.0);
    result.opticalDepth = float3(0.0, 0.0, 0.0);
    result.rayDistanceKm = 0.0;

    viewDir = normalize(viewDir);
    sunDir = normalize(sunDir);

    float t0 = 0.0;
    float t1 = 0.0;
    if (!WE_IntersectSphere(origin, viewDir, params.atmosphereRadius, t0, t1))
        return result;

    const float start = max(t0, 0.0);
    const float end = t1;
    result.rayDistanceKm = max(end - start, 0.0);
    const float stepSize = result.rayDistanceKm / float(WE_ATMOSPHERE_STEPS);

    float3 sumRayleigh = float3(0.0, 0.0, 0.0);
    float3 sumMie = float3(0.0, 0.0, 0.0);
    float3 opticalRayleigh = float3(0.0, 0.0, 0.0);
    float3 opticalMie = float3(0.0, 0.0, 0.0);
    float3 opticalOzone = float3(0.0, 0.0, 0.0);

    const float cosTheta = dot(viewDir, sunDir);
    const float phaseR = WE_RayleighPhase(cosTheta);
    const float phaseM = WE_MiePhase(cosTheta, params.mieAnisotropy);

    [loop]
    for (int i = 0; i < WE_ATMOSPHERE_STEPS; ++i)
    {
        const float t = start + (float(i) + 0.5) * stepSize;
        const float3 samplePos = origin + viewDir * t;
        const float heightKm = max(length(samplePos) - params.planetRadius, 0.0);

        const float rd = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
        const float md = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);
        const float od = WE_OzoneDensity(heightKm);

        opticalRayleigh += params.rayleighCoeff * rd * stepSize;
        opticalMie += params.mieCoeff * md * stepSize;
        opticalOzone += params.ozoneCoeff * od * stepSize;

        const float3 sunT = WE_IntegrateSunTransmittance(samplePos, sunDir, params);
        const float3 tau = opticalRayleigh + opticalMie + opticalOzone;
        const float3 atten = exp(-tau) * sunT;

        sumRayleigh += atten * rd * stepSize;
        sumMie += atten * md * stepSize;
    }

    result.transmittanceToCamera = exp(-(opticalRayleigh + opticalMie + opticalOzone));
    result.opticalDepth = opticalRayleigh + opticalMie + opticalOzone;

    const float3 tauView = opticalRayleigh + opticalMie;
    const float3 multiScatter = (1.0 - exp(-tauView * 0.08))
        * params.rayleighCoeff * params.multiScatterStrength;

    const float3 sunRadiance = params.sunColor * params.sunIntensity;
    result.rayleighContribution = WE_SKY_RADIANCE_SCALE * sunRadiance * sumRayleigh * phaseR * params.rayleighCoeff;
    result.mieContribution = WE_SKY_RADIANCE_SCALE * sunRadiance * sumMie * phaseM * params.mieCoeff;
    result.multiScatterContribution = WE_SKY_RADIANCE_SCALE * sunRadiance * multiScatter;
    result.skyRadiance = result.rayleighContribution + result.mieContribution + result.multiScatterContribution;
    return result;
}

float3 WE_IntegrateInscattering(
    float3 viewDir,
    float3 sunDir,
    float3 origin,
    WE_AtmosphereParams params,
    out float3 transmittanceToCamera)
{
    const WE_InscatteringResult detailed = WE_IntegrateInscatteringDetailed(viewDir, sunDir, origin, params);
    transmittanceToCamera = detailed.transmittanceToCamera;
    return detailed.skyRadiance;
}

float3 WE_ComputeSunDisk(float3 viewDir, float3 sunDir, float intensity, float3 sunColor, float angularRadius)
{
    const float disk = WE_ComputeSunDiskMask(viewDir, sunDir, max(angularRadius, WE_SUN_ANGULAR_RADIUS));
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    // Match inscattering radiance scale; mask confines this to ~0.53 degrees.
    return sunLinear * max(intensity, 0.0) * WE_SKY_RADIANCE_SCALE * disk;
}

#endif // WE_ATMOSPHERE_INTEGRATOR_HLSLI
