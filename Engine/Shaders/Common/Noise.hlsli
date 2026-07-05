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

float WE_Hash33(float3 p)
{
    float3 p3 = frac(float3(p.xyz) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float WE_ValueNoise3D(float3 p)
{
    const float3 i = floor(p);
    const float3 f = frac(p);
    const float3 u = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(lerp(WE_Hash33(i + float3(0, 0, 0)), WE_Hash33(i + float3(1, 0, 0)), u.x),
             lerp(WE_Hash33(i + float3(0, 1, 0)), WE_Hash33(i + float3(1, 1, 0)), u.x), u.y),
        lerp(lerp(WE_Hash33(i + float3(0, 0, 1)), WE_Hash33(i + float3(1, 0, 1)), u.x),
             lerp(WE_Hash33(i + float3(0, 1, 1)), WE_Hash33(i + float3(1, 1, 1)), u.x), u.y),
        u.z);
}

float WE_FBM3D(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    [loop]
    for (int i = 0; i < octaves; ++i)
    {
        value += amplitude * WE_ValueNoise3D(p * frequency);
        frequency *= 2.03;
        amplitude *= 0.5;
    }
    return value;
}

#endif // WE_NOISE_HLSLI
