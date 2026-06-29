#ifndef WE_SSR_HLSLI
#define WE_SSR_HLSLI

#include "../Common/Math.hlsli"

float3 WE_TraceSSR(float3 viewPos, float3 viewNormal, float3 viewReflect, float maxDistance)
{
    (void)viewPos; (void)viewNormal; (void)viewReflect; (void)maxDistance;
    return float3(0, 0, 0);
}

#endif // WE_SSR_HLSLI
