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

float2 WE_Hash22(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float3 WE_Hash33v(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
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

// Ridged FBM — sharper ridges useful for eroded cloud edges / cirrus streaks.
float WE_RidgedFBM3D(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float weight = 1.0;
    [loop]
    for (int i = 0; i < octaves; ++i)
    {
        float n = 1.0 - abs(WE_ValueNoise3D(p * frequency) * 2.0 - 1.0);
        n *= n * weight;
        value += n * amplitude;
        weight = saturate(n * 2.0);
        frequency *= 2.11;
        amplitude *= 0.5;
    }
    return saturate(value);
}

// 2D Worley (cellular) — low-frequency cloud-patch layout in XZ.
float WE_Worley2D(float2 p)
{
    const float2 i = floor(p);
    const float2 f = frac(p);
    float minDist = 1.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            const float2 g = float2(x, y);
            const float2 o = WE_Hash22(i + g);
            const float2 d = g + o - f;
            minDist = min(minDist, dot(d, d));
        }
    }
    return saturate(sqrt(minDist));
}

// 3D Worley — billowy cumulus cells (looped to keep SPIR-V size reasonable).
float WE_Worley3D(float3 p)
{
    const float3 i = floor(p);
    const float3 f = frac(p);
    float minDist = 1.0;
    [loop]
    for (int z = -1; z <= 1; ++z)
    {
        [loop]
        for (int y = -1; y <= 1; ++y)
        {
            [loop]
            for (int x = -1; x <= 1; ++x)
            {
                const float3 g = float3(x, y, z);
                const float3 o = WE_Hash33v(i + g);
                const float3 d = g + o - f;
                minDist = min(minDist, dot(d, d));
            }
        }
    }
    return saturate(sqrt(minDist));
}

#endif // WE_NOISE_HLSLI
