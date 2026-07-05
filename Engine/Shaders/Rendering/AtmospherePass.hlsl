#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "SkyAtmosphere.hlsli"

cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

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
    const float3 viewDir = WE_UnprojectDirection(input.uv, view, proj);
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    float3 skyLinear = WE_SampleSkyAtmosphere(
        viewDir, sunDirection, cameraPos, worldOrigin,
        sunColor, sunIntensity,
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));

    skyLinear = WE_SanitizeHdrColor(skyLinear);
    return float4(skyLinear, 1.0);
}
