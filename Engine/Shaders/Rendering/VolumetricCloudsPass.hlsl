#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "VolumetricClouds.hlsli"

#include "../Common/CameraBuffer.hlsli"

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
    o.position = float4(pos, 1.0, 1.0);
    o.uv = uv * 0.5;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableClouds < 0.5)
        discard;

    const float3 viewDir = WE_UnprojectDirection(input.uv, view, proj, cameraPos);
    const float3 sunDir = normalize(-sunDirection);
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    const float3 cloudAlbedo = WE_sRGBToLinear(saturate(cloudColor));

    float opacity = 0.0;
    const float3 clouds = WE_RaymarchClouds(
        cameraPos, viewDir, worldOrigin,
        sunDir, sunLinear, sunIntensity,
        cloudAltitude, cloudCoverage, cloudExtinction, cloudAlbedo,
        opacity);

    // Keep linear HDR — PostExposure applies exposure, tonemap, and sRGB conversion once.
    const float3 color = WE_SanitizeHdrColor(clouds);
    return float4(color, opacity);
}
