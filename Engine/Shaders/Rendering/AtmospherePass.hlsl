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
    // Validation stages — no atmosphere math.
    if (atmosphereDebugMode == 101)
        return float4(1.0, 0.0, 0.0, 1.0);
    if (atmosphereDebugMode == 102)
        return float4(0.0, 1.0, 0.0, 1.0);
    if (atmosphereDebugMode == 103)
        return float4(0.0, 0.0, 1.0, 1.0);
    if (atmosphereDebugMode == 104)
        return float4(0.2, 0.4, 1.0, 1.0);

    const float3 viewDir = normalize(WE_UnprojectDirection(input.uv, view, proj, cameraPos));
    const float3 sunDir = normalize(-sunDirection);

    if (atmosphereDebugMode == 1)
        return float4(saturate(viewDir * 0.5 + 0.5), 1.0);
    if (atmosphereDebugMode == 2)
        return float4(saturate(sunDir * 0.5 + 0.5), 1.0);

    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunColor, float3(0.0, 0.0, 0.0)), sunIntensity,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));

    const float3 origin = WE_GetAtmosphereOrigin(cameraPos, worldOrigin, params.planetRadius, params.eyeAltitude);
    WE_InscatteringResult inscatter = WE_IntegrateInscatteringDetailed(viewDir, sunDir, origin, params);

    float3 skyLinear = float3(0.0, 0.0, 0.0);

    if (atmosphereDebugMode == 4)
        skyLinear = inscatter.opticalDepth;
    else if (atmosphereDebugMode == 5)
        skyLinear = inscatter.transmittanceToCamera;
    else if (atmosphereDebugMode == 6)
        skyLinear = inscatter.rayleighContribution;
    else if (atmosphereDebugMode == 7)
        skyLinear = inscatter.mieContribution;
    else if (atmosphereDebugMode == 9)
        skyLinear = WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, max(sunColor, float3(0.0, 0.0, 0.0)), params.sunAngularRadius);
    else
    {
        skyLinear = inscatter.skyRadiance;
        skyLinear += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, max(sunColor, float3(0.0, 0.0, 0.0)), params.sunAngularRadius);
    }

    skyLinear = WE_SanitizeHdrColor(skyLinear);
    return float4(skyLinear, 1.0);
}
