#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "AtmosphereLUT.hlsli"

cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

Texture2D    transmittanceLUT : register(t0, space2);
Texture2D    multiScatterLUT  : register(t1, space2);
Texture2D    skyViewLUT       : register(t2, space2);
Texture2D    aerialLUT        : register(t3, space2);
SamplerState lutSampler       : register(s0, space2);

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

    const float3 viewDirRaw = WE_UnprojectDirection(input.uv, view, proj, cameraPos);
    const float viewDirLen = length(viewDirRaw);
    const float3 viewDir = viewDirRaw / max(viewDirLen, 1e-6);
    const float3 sunDir = normalize(-sunDirection);
    const float sunDirLen = length(sunDirection);
    const float viewSunDot = dot(viewDir, sunDir);

    if (atmosphereDebugMode == 1)
        return float4(saturate(viewDir * 0.5 + 0.5), 1.0);
    if (atmosphereDebugMode == 2)
        return float4(saturate(sunDir * 0.5 + 0.5), 1.0);
    if (atmosphereDebugMode == 3)
        return float4(saturate(viewSunDot * 0.5 + 0.5), saturate(abs(viewSunDot)), 1.0 - saturate(abs(viewSunDot)), 1.0);
    if (atmosphereDebugMode == 13)
        return float4(saturate(viewDirLen), saturate(viewDirLen), saturate(viewDirLen), 1.0);

    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunColor, float3(0.0, 0.0, 0.0)), sunIntensity,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));

    const float3 origin = WE_GetAtmosphereOrigin(cameraPos, worldOrigin, params.planetRadius, params.eyeAltitude);
    const float heightKm = max(length(origin) - params.planetRadius, 0.0);
    const float viewHeightKm = length(origin);

    float viewZenithAngle = 0.0;
    const float2 skyViewUV = WE_SkyViewLUTCoord(viewDir, sunDir, viewZenithAngle);
    const float2 transmittanceUV = WE_LutTransmittanceParamsToUv(viewHeightKm, viewDir.y, params);
    const float3 skyViewRGB = WE_SampleSkyViewLUT(skyViewLUT, lutSampler, viewDir, sunDir);
    const float3 transmittanceRGB = WE_SampleTransmittanceLUT(transmittanceLUT, lutSampler, heightKm, viewSunDot, params);

    if (atmosphereDebugMode == 10)
        return float4(skyViewUV, 0.0, 1.0);
    if (atmosphereDebugMode == 11)
        return float4(saturate(skyViewRGB / max(sunIntensity, 1.0)), 1.0);
    if (atmosphereDebugMode == 12)
        return float4(transmittanceUV, 0.0, 1.0);

    const float sunDiskMask = WE_ComputeSunDiskMask(viewDir, sunDir, params.sunAngularRadius);
    if (atmosphereDebugMode == 14)
        return float4(sunDiskMask, sunDiskMask, sunDiskMask, 1.0);

    WE_InscatteringResult inscatter = WE_IntegrateInscatteringDetailed(viewDir, sunDir, origin, params);
    const float3 sunRGB = WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, max(sunColor, float3(0.0, 0.0, 0.0)), params.sunAngularRadius);

    float3 skyLinear = float3(0.0, 0.0, 0.0);

    if (atmosphereDebugMode == 4)
        skyLinear = inscatter.opticalDepth;
    else if (atmosphereDebugMode == 5)
        skyLinear = inscatter.transmittanceToCamera;
    else if (atmosphereDebugMode == 6)
        skyLinear = inscatter.rayleighContribution;
    else if (atmosphereDebugMode == 7)
        skyLinear = inscatter.mieContribution;
    else if (atmosphereDebugMode == 8)
        skyLinear = inscatter.multiScatterContribution;
    else if (atmosphereDebugMode == 9)
        skyLinear = sunRGB;
    else if (atmosphereDebugMode == 15)
    {
        // Live diagnostic grid: encodes probe values as colors across the viewport.
        const float2 grid = floor(input.uv * float2(4.0, 3.0));
        const float cell = grid.x + grid.y * 4.0;
        if (cell < 0.5)
            skyLinear = viewDir * 0.5 + 0.5;
        else if (cell < 1.5)
            skyLinear = sunDir * 0.5 + 0.5;
        else if (cell < 2.5)
            skyLinear = float3(viewSunDot * 0.5 + 0.5, viewSunDot * 0.5 + 0.5, viewSunDot * 0.5 + 0.5);
        else if (cell < 3.5)
            skyLinear = float3(skyViewUV, 0.0);
        else if (cell < 4.5)
            skyLinear = float3(transmittanceUV, 0.0);
        else if (cell < 5.5)
            skyLinear = saturate(skyViewRGB / max(sunIntensity, 1.0));
        else if (cell < 6.5)
            skyLinear = saturate(inscatter.rayleighContribution / max(sunIntensity, 1.0));
        else if (cell < 7.5)
            skyLinear = saturate(inscatter.mieContribution / max(sunIntensity, 1.0));
        else if (cell < 8.5)
            skyLinear = saturate(sunRGB / max(sunIntensity, 1.0));
        else if (cell < 9.5)
            skyLinear = saturate((skyViewRGB + sunRGB) / max(sunIntensity, 1.0));
        else if (cell < 10.5)
            skyLinear = float3(saturate(viewDirLen), 0.0, 0.0);
        else
            skyLinear = float3(sunDiskMask, sunDiskMask, sunDiskMask);
    }
    else
    {
        skyLinear = WE_SampleSkyAtmosphereLUT(
            viewDir, sunDirection, cameraPos, worldOrigin,
            sunColor, sunIntensity,
            rayleigh, mieScattering, ozone, mieAnisotropy,
            planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
            max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS),
            transmittanceLUT, multiScatterLUT, skyViewLUT, aerialLUT, lutSampler);
    }

    skyLinear = WE_SanitizeHdrColor(skyLinear);
    // Encode linear sky to sRGB for the UNORM swapchain.
    return float4(WE_LinearToSRGB(skyLinear), 1.0);
}
