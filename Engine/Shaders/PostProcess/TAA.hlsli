#ifndef WE_TAA_HLSLI
#define WE_TAA_HLSLI

#include "../Common/Math.hlsli"

float3 WE_ApplyTAA(float3 current, float3 history, float blendFactor)
{
    return lerp(history, current, saturate(blendFactor));
}

#endif // WE_TAA_HLSLI
