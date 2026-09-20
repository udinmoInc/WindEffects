#ifndef WE_FOUNDATION_PROCEDURAL_SKY_HLSL
#define WE_FOUNDATION_PROCEDURAL_SKY_HLSL

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/Math.hlsli"
#include "../Rendering/AtmosphereIntegrator.hlsli"
#include "../Common/EnvironmentLighting.hlsli"

// Authoritative visible sky.
// Sun disk = angular cone about toSun (circular on the celestial sphere).
//
// CRITICAL: view rays must be reconstructed PER PIXEL from NDC + invViewProj.
// Interpolating normalize(dir) across the fullscreen triangle warps equal-angle
// isocontours into diagonal ellipses (visible on the hard sun disk).
//
// Pipeline: HDR sky → exposure (EV100) → filmic tonemap → ONE Linear→sRGB
//
// Debug (cameraPadding / atmosphereDebugMode):
//   0 final, 2 disk only, 10 atmosphere only, 7 SkyLight irr, 6 sun dir, 8 T

struct VSOutput
{
    float4 position : SV_Position;
    // Clip-space XY for the covering triangle. Linear in screen ≡ NDC here (w=1).
    float2 clipXY   : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float4 clip = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    VSOutput o;
    o.position = float4(clip.xy, 1.0, 1.0);
    o.clipXY = clip.xy;
    return o;
}

float3 WE_SkyViewDirFromClip(float2 clipXY)
{
    float4 world = mul(invViewProj, float4(clipXY, 1.0, 1.0));
    return normalize(world.xyz / max(world.w, 1e-6) - cameraPos);
}

float3 EncodeDisplay(float3 hdrLinear, int debugMode)
{
    if (debugMode == 5)
        return saturate(hdrLinear * 0.04);

    const float exposureScale =
        WE_ExposureFromEV100(exposureEV - 8.0) * exp2(exposureCompensation * 0.25);

    if (debugMode == 9)
        return saturate(hdrLinear * exposureScale * 0.12);

    if (pipelineBypassToneMapping > 0)
        return saturate(hdrLinear * exposureScale);

    const float3 tonemapped = WE_ApplyFilmicTonemap(hdrLinear, exposureScale);
    return WE_LinearToSRGB(tonemapped);
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Per-pixel ray — do not use an interpolated direction from the VS.
    const float3 dir = WE_SkyViewDirFromClip(input.clipXY);
    const int debugMode = max((int)round(cameraPadding), atmosphereDebugMode);
    const float3 toSun = normalize(-sunDirection);

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        atmosphereRayleigh, mieScattering, ozoneAbsorption, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunColor, sunIntensity, sunAngularRadius);

    const float3 origin = WE_GetAtmosphereOrigin(
        cameraPos, worldOrigin, params.planetRadius, params.eyeAltitude);

    WE_InscatteringResult insc = WE_IntegrateInscatteringDetailed(dir, toSun, origin, params);
    float3 atmosphereHdr = WE_SanitizeHdrColor(insc.skyRadiance);

    if (dir.y < 0.0)
    {
        const float ground = saturate(-dir.y);
        const float3 groundTint = float3(0.10, 0.11, 0.12) * (0.35 + 0.65 * saturate(toSun.y));
        atmosphereHdr = lerp(atmosphereHdr, groundTint, smoothstep(0.0, 0.35, ground));
    }

    // One angular sun disk (circular in angle; stays round on screen when rays are correct).
    float3 sunDiskHdr = float3(0.0, 0.0, 0.0);
    if (enableSunDisk > 0.5 && toSun.y > -0.04)
    {
        sunDiskHdr = WE_ComputeSunDisk(
            dir, toSun, params.sunIntensity, params.sunColor, params.sunAngularRadius);
        sunDiskHdr *= insc.transmittanceToCamera;
        sunDiskHdr = WE_SanitizeHdrColor(sunDiskHdr);
    }

    WE_EnvLightSample envLite = WE_EvalEnvironmentLighting(dir);

    if (debugMode == 10)
        return float4(EncodeDisplay(atmosphereHdr, 0), 1.0);
    if (debugMode == 2)
        return float4(EncodeDisplay(sunDiskHdr, 0), 1.0);
    if (debugMode == 11)
        return float4(saturate((atmosphereHdr + sunDiskHdr) * 0.06), 1.0);
    if (debugMode == 7)
        return float4(EncodeDisplay(envLite.skyIrradiance, 0), 1.0);
    if (debugMode == 12)
        return float4(EncodeDisplay(atmosphereHdr, 0), 1.0);
    if (debugMode == 1)
        return float4(saturate((atmosphereHdr + sunDiskHdr) * 0.06), 1.0);
    if (debugMode == 6)
        return float4(toSun * 0.5 + 0.5, 1.0);
    if (debugMode == 8)
        return float4(saturate(insc.transmittanceToCamera), 1.0);

    return float4(EncodeDisplay(atmosphereHdr + sunDiskHdr, debugMode), 1.0);
}

#endif
