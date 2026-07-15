#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/CloudFrameBuffer.hlsli"
#include "../Common/CameraBuffer.hlsli"

Texture2D<float4> cloudResolved : register(t1, space2);
Texture2D<float> sceneDepth : register(t4, space2);
SamplerState cloudSampler : register(s3, space2);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 pos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    VSOutput o;
    o.position = float4(pos, 0.0, 1.0);
    o.uv = uv * 0.5;
    return o;
}

float4 WE_BilateralUpsample(float2 uv)
{
    const float4 center = cloudResolved.SampleLevel(cloudSampler, uv, 0);
    // Do not bleed alpha from neighbors into empty sky.
    if (center.a <= 1e-4)
        return float4(0.0, 0.0, 0.0, 0.0);

    float4 accum = center * 2.0;
    float weightSum = 2.0;

    const float2 offsets[4] = {
        float2( cloudInvCloudResolution.x, 0.0),
        float2(-cloudInvCloudResolution.x, 0.0),
        float2(0.0,  cloudInvCloudResolution.y),
        float2(0.0, -cloudInvCloudResolution.y)
    };

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        const float4 s = cloudResolved.SampleLevel(cloudSampler, uv + offsets[i], 0);
        if (s.a <= 1e-4)
            continue;
        const float w = exp(-abs(s.a - center.a) * 14.0);
        accum += s * w;
        weightSum += w;
    }
    return accum / max(weightSum, 1e-3);
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableClouds < 0.5)
        discard;

    const float4 clouds = WE_BilateralUpsample(input.uv);
    const int cloudDebug = atmosphereDebugMode;

    // Occlude with scene depth: opaque geometry in front kills cloud contribution.
    const float z = sceneDepth.SampleLevel(cloudSampler, input.uv, 0).r;
    if (z < 0.9999 && clouds.a > 1e-4)
    {
        const float2 ndc = input.uv * 2.0 - 1.0;
        const float3 worldPos = WE_UnprojectPoint(ndc.x, ndc.y, z, view, proj);
        const float sceneT = length(worldPos - cameraPos);
        // Conservative: if scene is very close, fully kill; soft fade for mid-range.
        if (sceneT < 50.0)
            discard;
    }

    if (cloudDebug == 25)
        return float4(WE_LinearToSRGB(saturate(clouds.rgb * 0.5)), saturate(clouds.a + 0.05));
    if (cloudDebug == 26 || cloudDebug == 31)
        return float4(clouds.aaa, saturate(clouds.a + 0.05));

    if (clouds.a <= 1e-4)
        discard;

    const float3 compressed = clouds.rgb / (1.0 + clouds.rgb * 0.35);
    const float3 color = WE_LinearToSRGB(WE_SanitizeHdrColor(compressed));

    if (cloudDebug == 27)
        return float4(color, 1.0);

    return float4(color, saturate(clouds.a));
}
