// PBR material functions — forward/deferred/clustered lighting integration point.
#ifndef WE_PBR_LIT_HLSLI
#define WE_PBR_LIT_HLSLI

#include "../Common/Platform.hlsli"

struct PBRSurface
{
    float3 albedo;
    float  metallic;
    float  roughness;
    float3 normal;
    float3 emissive;
};

// Placeholder for future GGX BRDF evaluation.
float3 WE_EvaluatePBR(PBRSurface surface, float3 lightDir, float3 viewDir)
{
    return surface.albedo * 0.25;
}

#endif // WE_PBR_LIT_HLSLI
