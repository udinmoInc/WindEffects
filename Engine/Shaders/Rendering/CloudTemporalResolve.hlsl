#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/CloudFrameBuffer.hlsli"
#include "../Common/CameraBuffer.hlsli"

Texture2D<float4> cloudScratch : register(t1, space2);
Texture2D<float4> cloudHistory : register(t2, space2);
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

float2 WE_CloudReprojectUV(float2 uv)
{
    const float3 viewDir = WE_UnprojectDirection(uv, view, proj, cameraPos);
    const float bottom = worldOrigin.y + cloudBottomAltitude;
    const float top = worldOrigin.y + max(cloudTopAltitude, cloudBottomAltitude + 50.0);
    float t0 = (bottom - cameraPos.y) / (abs(viewDir.y) < 1e-5 ? 1e-5 : viewDir.y);
    float t1 = (top - cameraPos.y) / (abs(viewDir.y) < 1e-5 ? 1e-5 : viewDir.y);
    if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; }
    t0 = max(t0, 0.0);
    if (t1 < 0.0)
        return uv;

    const float midT = 0.5 * (t0 + t1);
    const float3 worldPos = cameraPos + viewDir * midT;
    const float4 prevClip = mul(cloudPrevViewProj, float4(worldPos, 1.0));
    if (prevClip.w <= 1e-4)
        return uv;
    const float2 prevNdc = prevClip.xy / prevClip.w;
    return prevNdc * float2(0.5, -0.5) + 0.5;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float4 current = cloudScratch.SampleLevel(cloudSampler, input.uv, 0);

    // Never resurrect history fog into clear sky pixels — prevents fullscreen veil ghosting.
    if (current.a <= 1e-4)
        return float4(0.0, 0.0, 0.0, 0.0);

    const float2 histUV = WE_CloudReprojectUV(input.uv);
    const bool histInBounds = all(histUV >= 0.0) && all(histUV <= 1.0);

    float4 history = current;
    if (histInBounds && cloudHistoryValidFlag > 0.5)
        history = cloudHistory.SampleLevel(cloudSampler, histUV, 0);

    const float camDelta = length(cameraPos - cloudPrevCameraPos);
    const float cutWeight = saturate(1.0 - camDelta / 25.0);
    const float opacityDelta = abs(current.a - history.a);
    const float edgeWeight = saturate(1.0 - opacityDelta * 4.0);
    float blend = saturate(cloudTemporalBlendAmt) * cloudHistoryValidFlag * cutWeight * edgeWeight;
    if (!histInBounds || history.a <= 1e-4)
        blend = 0.0;

    const int cloudDebug = atmosphereDebugMode;
    if (cloudDebug == 23)
        return float4(history.rgb, saturate(history.a + 0.05));
    if (cloudDebug == 24)
        return float4(blend, cloudHistoryValidFlag, cutWeight, 1.0);

    float3 neighborhoodMin = current.rgb;
    float3 neighborhoodMax = current.rgb;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            const float2 offset = float2(x, y) * cloudInvCloudResolution;
            const float3 s = cloudScratch.SampleLevel(cloudSampler, input.uv + offset, 0).rgb;
            neighborhoodMin = min(neighborhoodMin, s);
            neighborhoodMax = max(neighborhoodMax, s);
        }
    }
    history.rgb = clamp(history.rgb, neighborhoodMin, neighborhoodMax);

    return float4(
        lerp(current.rgb, history.rgb, blend),
        lerp(current.a, history.a, blend * 0.85));
}
