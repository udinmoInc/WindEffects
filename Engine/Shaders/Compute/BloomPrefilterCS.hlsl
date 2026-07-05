#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../PostProcess/Bloom.hlsli"

Texture2D<float4> sceneColor : register(t0, space1);
RWTexture2D<float4> bloomTarget : register(u0, space2);

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint bloomW = 0;
    uint bloomH = 0;
    bloomTarget.GetDimensions(bloomW, bloomH);
    if (dtid.x >= bloomW || dtid.y >= bloomH)
        return;

    const uint sceneW = bloomW * 2;
    const uint sceneH = bloomH * 2;
    const uint2 base = dtid.xy * 2;

    float3 sum = float3(0.0, 0.0, 0.0);
    [unroll]
    for (uint y = 0; y < 2; ++y)
    {
        [unroll]
        for (uint x = 0; x < 2; ++x)
        {
            const uint2 coord = min(base + uint2(x, y), uint2(sceneW - 1, sceneH - 1));
            sum += WE_SanitizeHdrColor(sceneColor[coord].rgb);
        }
    }

    const float3 avg = sum * 0.25;
    const float3 bloom = WE_ExtractBloom(avg, bloomThreshold);
    bloomTarget[dtid.xy] = float4(bloom, 1.0);
}
