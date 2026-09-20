#ifndef WE_LIGHTING_HLSLI
#define WE_LIGHTING_HLSLI

#include "../Common/Math.hlsli"
#include "../Common/EnvironmentLighting.hlsli"
#include "../Materials/PBR_Lit.hlsli"
#include "../Rendering/Shadow.hlsli"

struct WE_DirectionalLight
{
    float3 direction;
    float  intensity;
    float3 color;
    float  padding;
};

float3 WE_EvaluateDirectionalLight(WE_DirectionalLight light, float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 L = normalize(-light.direction);
    PBRSurface surface;
    surface.albedo = albedo;
    surface.metallic = metallic;
    surface.roughness = roughness;
    surface.normal = N;
    surface.emissive = float3(0, 0, 0);
    return WE_EvaluatePBR(surface, L, V) * light.color * light.intensity;
}

/// Environment/sky irradiance for surfaces — same atmosphere model as the visible sky.
float3 WE_EvaluateEnvironmentDiffuse(float3 N)
{
    const float3 upper = max(skyAmbientColor, 0.0);
    const float3 lower = max(skyLightLowerColor, 0.0);
    const float w = saturate(N.y * 0.5 + 0.5);
    float3 irr = lerp(lower, upper, w);
    if (dot(upper, float3(1, 1, 1)) < 1e-6)
    {
        irr = WE_EvalSkyIrradianceUpper(
            sunDirection, sunColor, sunIntensity,
            atmosphereRayleigh, mieScattering, ozoneAbsorption,
            multiScatterStrength, skyLightIntensity);
        irr = lerp(lower, irr, w);
    }
    return irr;
}

/// worldPos + viewDepth enable CSM when WE_SUN_SHADOWS_ENABLED=1 and buffers are bound.
float3 WE_EvaluateSurfaceLightingShadowed(
    WE_DirectionalLight sun,
    float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness,
    float3 worldPos,
    float viewDepth)
{
    float3 direct = WE_EvaluateDirectionalLight(sun, N, V, albedo, metallic, roughness);
    const float3 toSun = normalize(-sun.direction);
    const float3 sunT = WE_EnvSunTransmittance(
        toSun, atmosphereRayleigh, mieScattering, ozoneAbsorption);
    direct *= sunT;

    // Authoritative Sun shadows (CSM). Same sun.direction as atmosphere / disk.
    const float sunVis = WE_EvaluateSunVisibility(worldPos, N, viewDepth);
    direct *= sunVis;

    const float3 envDiffuse = WE_EvaluateEnvironmentDiffuse(N) * albedo * (1.0 - metallic);
    return direct + envDiffuse;
}

float3 WE_EvaluateSurfaceLighting(
    WE_DirectionalLight sun,
    float3 N,
    float3 V,
    float3 albedo,
    float metallic,
    float roughness)
{
    return WE_EvaluateSurfaceLightingShadowed(
        sun, N, V, albedo, metallic, roughness, float3(0, 0, 0), 0.0);
}

#endif // WE_LIGHTING_HLSLI
