#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "VolumetricClouds.hlsli"

#include "../Common/CameraBuffer.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float3 viewDir  : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float4 clip = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    float4 world = mul(invViewProj, clip);
    float3 viewDir = normalize(world.xyz / world.w - cameraPos);

    VSOutput o;
    o.position = float4(clip.xy, 0.0, 1.0);
    o.viewDir = viewDir;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableClouds < 0.5)
        discard;

    const float3 viewDir = normalize(input.viewDir);
    const float3 sunDir = normalize(-sunDirection);
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));

    float opacity = 0.0;
    const float3 cloudsLinear = WE_RaymarchClouds(
        cameraPos, viewDir, worldOrigin,
        sunDir, sunLinear, sunIntensity,
        opacity);

    if (opacity <= 1e-4)
        discard;

    // Foundation path is display-referred (ProceduralSky writes sRGB). Encode clouds the same way.
    const float3 compressed = cloudsLinear / (1.0 + cloudsLinear * 0.35);
    const float3 color = WE_LinearToSRGB(WE_SanitizeHdrColor(compressed));
    return float4(color, saturate(opacity));
}
