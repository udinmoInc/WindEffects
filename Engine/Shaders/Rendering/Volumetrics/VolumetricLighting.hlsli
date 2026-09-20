#ifndef WE_VOLUMETRIC_LIGHTING_HLSLI
#define WE_VOLUMETRIC_LIGHTING_HLSLI

// Unified volumetric light transport — consumes shared EnvironmentLighting.
// Providers supply VolumeSample; this module evaluates sun + sky + multi-scatter.

#include "VolumetricScattering.hlsli"
#include "../../Common/EnvironmentLighting.hlsli"

struct VolumetricLightResult
{
    float3 direct;      // sun / directional (HDR)
    float3 ambient;     // sky / environment (HDR)
    float3 multiScatter;
};

/// Primary volumetric lighting evaluation (Beer–Lambert transmittance already in opticalDepth).
VolumetricLightResult WE_EvaluateVolumetricLighting(
    float3 rayDir,
    float3 toSun,
    float3 sunColor,
    float sunIntensity,
    float3 skyAmbient,
    float skyIntensity,
    float ambientScale,
    float heightFraction,
    float opticalDepth,
    float absorption,
    float cosTheta,
    float powderStrength,
    float phaseGForward,
    float phaseGBack,
    float silverIntensity,
    float silverSpread,
    float albedo,
    float multiScatterStrength)
{
    VolumetricLightResult r;

    // Prefer environment-derived sun transmittance when EnvironmentBuffer is bound.
    const float3 sunT = WE_EnvSunTransmittance(
        toSun, atmosphereRayleigh, mieScattering, ozoneAbsorption);
    const float3 sunCol = max(sunColor, 0.0.xxx) * max(sunIntensity, 0.0) * sunT;

    // Sky irradiance: prefer shared eval; fall back to UBO ambient.
    float3 envIrr = WE_EvalSkyIrradianceUpper(
        sunDirection, sunColor, sunIntensity,
        atmosphereRayleigh, mieScattering, ozoneAbsorption,
        multiScatterStrength, skyLightIntensity);
    envIrr = max(envIrr, max(skyAmbient, 0.0.xxx) * max(skyIntensity, 0.0));
    const float3 ambientCol = envIrr * ambientScale * 0.32;

    const float beerPowder = WE_BeerPowderMultiScatter(
        opticalDepth, absorption, cosTheta, powderStrength);
    const float phase = WE_DualLobeHG(cosTheta, phaseGForward, phaseGBack, 0.55);
    const float silver = WE_SilverLining(cosTheta, silverIntensity, silverSpread);
    const float coreFill = saturate(1.0 - WE_BeerLaw(opticalDepth, absorption));

    const float heightAmb = lerp(0.55, 1.35, saturate(heightFraction));
    r.ambient = ambientCol * heightAmb * (1.0 + coreFill * 0.20);

    float directLight = beerPowder * phase * silver;
    float3 sunScatter = sunCol * max(directLight, 0.0) * float3(1.05, 1.02, 0.98);
    // Soft highlight compression — keeps HDR headroom (not display tonemap).
    sunScatter = sunScatter / (1.0 + sunScatter * 0.18);
    r.direct = sunScatter * max(albedo, 0.0);

    // Modular multiple-scattering approximation (interior fill).
    const float ms = saturate(multiScatterStrength) * coreFill;
    r.multiScatter = (r.ambient + r.direct * 0.35) * ms * 0.45;

    (void)rayDir;
    return r;
}

// Backward-compatible overload (defaults multi-scatter from atmosphere UBO).
VolumetricLightResult WE_EvaluateVolumetricLighting(
    float3 rayDir,
    float3 toSun,
    float3 sunColor,
    float sunIntensity,
    float3 skyAmbient,
    float skyIntensity,
    float ambientScale,
    float heightFraction,
    float opticalDepth,
    float absorption,
    float cosTheta,
    float powderStrength,
    float phaseGForward,
    float phaseGBack,
    float silverIntensity,
    float silverSpread,
    float albedo)
{
    return WE_EvaluateVolumetricLighting(
        rayDir, toSun, sunColor, sunIntensity, skyAmbient, skyIntensity,
        ambientScale, heightFraction, opticalDepth, absorption, cosTheta,
        powderStrength, phaseGForward, phaseGBack, silverIntensity, silverSpread,
        albedo, multiScatterStrength);
}

float WE_PointLightAttenuation(float3 worldPos, float3 lightPos, float range)
{
    const float d = length(lightPos - worldPos);
    const float x = saturate(1.0 - d / max(range, 1e-3));
    return x * x;
}

float WE_SpotLightAttenuation(
    float3 worldPos,
    float3 lightPos,
    float3 lightDir,
    float range,
    float innerCos,
    float outerCos)
{
    const float attn = WE_PointLightAttenuation(worldPos, lightPos, range);
    const float3 toL = normalize(lightPos - worldPos);
    const float cosAng = dot(-toL, normalize(lightDir));
    const float cone = saturate((cosAng - outerCos) / max(innerCos - outerCos, 1e-4));
    return attn * cone * cone;
}

#endif // WE_VOLUMETRIC_LIGHTING_HLSLI
