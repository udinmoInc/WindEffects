#ifndef WE_ACES_HLSLI
#define WE_ACES_HLSLI

#include "../Common/Platform.hlsli"

// ACES fitted tonemapper — used by future HDR post-process passes.
float3 WE_TonemapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

#endif // WE_ACES_HLSLI
