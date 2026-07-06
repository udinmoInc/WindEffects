#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

Texture2D<float4> cloudScratch : register(t0, space2);
Texture2D<float4> cloudHistory : register(t1, space2);
SamplerState cloudSampler : register(s0, space2);

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

float4 PSMain(VSOutput input) : SV_Target
{
    const float4 current = cloudScratch.Sample(cloudSampler, input.uv);
    const float4 history = cloudHistory.Sample(cloudSampler, input.uv);
    const float blend = saturate(cloudTemporalBlend) * float(cloudHistoryValid);
    return float4(
        lerp(current.rgb, history.rgb, blend),
        lerp(current.a, history.a, blend));
}
