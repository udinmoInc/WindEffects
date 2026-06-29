#ifndef WE_NOISE_HLSLI
#define WE_NOISE_HLSLI

#include "Platform.hlsli"

float WE_Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float WE_BlueNoise(float2 p)
{
    float n  = WE_Hash12(p);
    float n2 = WE_Hash12(p + 17.31);
    float n3 = WE_Hash12(p * 1.7 + 43.17);
    return frac(n * 0.55 + n2 * 0.30 + n3 * 0.15);
}

// Interleaved gradient noise — excellent for banding removal in dark gradients.
float WE_InterleavedGradientNoise(float2 screenPos)
{
    return frac(52.9829189 * frac(dot(screenPos, float2(0.06711056, 0.00583715))));
}

#endif // WE_NOISE_HLSLI
