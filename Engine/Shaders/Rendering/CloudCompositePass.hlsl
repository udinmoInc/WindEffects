#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"

Texture2D<float4> cloudResolved : register(t0, space2);
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
    const float4 clouds = cloudResolved.Sample(cloudSampler, input.uv);
    const float3 color = WE_SanitizeHdrColor(clouds.rgb);
    return float4(color, clouds.a);
}
