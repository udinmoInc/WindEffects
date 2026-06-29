#ifndef WE_LIGHTING_HLSLI
#define WE_LIGHTING_HLSLI

#include "../Common/Math.hlsli"
#include "../Materials/PBR_Lit.hlsli"

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

#endif // WE_LIGHTING_HLSLI
