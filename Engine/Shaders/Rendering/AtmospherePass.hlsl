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

float3 WE_VisualizeScalar(float value, float scale)
{
    const float v = saturate(value / max(scale, 1e-5));
    return float3(v, v * v, 1.0 - v);
}

float3 WE_VisualizeVector(float3 value, float scale)
{
    return saturate(value / max(scale, 1e-5) * 0.5 + 0.5);
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float3 viewDir = normalize(WE_UnprojectDirection(input.uv, view, proj, cameraPos));
    const float3 sunDir = normalize(-sunDirection);
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunColor, float3(0.0, 0.0, 0.0)), sunIntensity,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));

    const float3 origin = WE_GetAtmosphereOrigin(cameraPos, worldOrigin, params.planetRadius, params.eyeAltitude);
    WE_InscatteringResult inscatter = WE_IntegrateInscatteringDetailed(viewDir, sunDir, origin, params);

    float3 skyLinear = inscatter.skyRadiance;
    skyLinear += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, max(sunColor, float3(0.0, 0.0, 0.0)), params.sunAngularRadius);

    if (atmosphereDebugMode == 1)
        skyLinear = WE_VisualizeVector(viewDir, 1.0);
    else if (atmosphereDebugMode == 2)
        skyLinear = WE_VisualizeVector(sunDir, 1.0);
    else if (atmosphereDebugMode == 3)
        skyLinear = WE_VisualizeScalar(inscatter.rayDistanceKm, params.atmosphereRadius - params.planetRadius);
    else if (atmosphereDebugMode == 4)
        skyLinear = WE_VisualizeScalar(dot(inscatter.opticalDepth, float3(0.2126, 0.7152, 0.0722)), 4.0);
    else if (atmosphereDebugMode == 5)
        skyLinear = WE_VisualizeVector(inscatter.transmittanceToCamera, 1.0);
    else if (atmosphereDebugMode == 6)
        skyLinear = WE_VisualizeVector(inscatter.rayleighContribution, 2.0);
    else if (atmosphereDebugMode == 7)
        skyLinear = WE_VisualizeVector(inscatter.mieContribution, 2.0);
    else if (atmosphereDebugMode == 8)
        skyLinear = inscatter.skyRadiance;

    skyLinear = WE_SanitizeHdrColor(skyLinear);
    return float4(skyLinear, 1.0);
}
