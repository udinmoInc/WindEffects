#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Rendering/AtmosphereLUT.hlsli"

Texture2D    sceneColorTexture : register(t0, space1);
SamplerState sceneSampler      : register(s0, space1);

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
    o.uv = uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 color = sceneColorTexture.Sample(sceneSampler, input.uv).rgb;
    const float ev = WE_ComputeExposureEV(sunDirection, exposureEV, exposureCompensation);
    const float exposureScale = WE_ExposureFromEV100(ev);
    color = WE_ApplyFilmicTonemap(color, exposureScale);
    color = WE_LinearToSRGB(color);
    return float4(color, 1.0);
}
