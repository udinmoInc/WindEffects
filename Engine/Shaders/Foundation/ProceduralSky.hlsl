#ifndef WE_FOUNDATION_PROCEDURAL_SKY_HLSL
#define WE_FOUNDATION_PROCEDURAL_SKY_HLSL

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

// Foundation sky: display-referred. Shares EnvironmentBuffer (sun) with clouds/meshes.
// cameraPadding = skyDebugMode:
// 0 final, 1 sky only, 2 sun-disk mask, 3 luminance, 4 no sun, 5 linear HDR,
// 6 sun direction RGB, 7 sun screen-pos marker, 8 aspect-correct radius only.

struct VSOutput
{
    float4 position : SV_Position;
    float3 viewDir  : TEXCOORD0;
    float2 ndc      : TEXCOORD1;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float4 clip = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    float4 world = mul(invViewProj, clip);
    float3 viewDir = normalize(world.xyz / world.w - cameraPos);

    VSOutput output;
    output.position = float4(clip.xy, 1.0, 1.0);
    output.viewDir = viewDir;
    output.ndc = clip.xy;
    return output;
}

float3 SkyGradient(float3 dir)
{
    float height = saturate(dir.y * 0.5 + 0.5);
    float3 horizon = float3(0.55, 0.62, 0.72);
    float3 zenith  = float3(0.12, 0.28, 0.58);
    float3 nadir   = float3(0.22, 0.24, 0.26);
    float3 upper = lerp(horizon, zenith, pow(height, 1.25));
    return lerp(nadir, upper, saturate(dir.y * 0.85 + 0.85));
}

// Map a world direction to NDC using the same view*proj as scene geometry.
float2 WE_DirectionToNdc(float3 worldDir)
{
    float4 viewSpace = mul(view, float4(worldDir, 0.0));
    float4 clip = mul(proj, viewSpace);
    // Behind / grazing camera: treat as invalid.
    if (clip.w <= 1e-5 || viewSpace.z >= 0.0)
        return float2(1e6, 1e6);
    return clip.xy / clip.w;
}

// Pixel-aspect correction: normalize NDC deltas so circles stay circular at any viewport ratio.
float2 WE_AspectCorrectNdcDelta(float2 deltaNdc)
{
    // proj[0][0] = f/aspect, proj[1][1] = ±f → ratio corrects X vs Y pixel stretch.
    const float sx = max(abs(proj[0][0]), 1e-5);
    const float sy = max(abs(proj[1][1]), 1e-5);
    deltaNdc.x *= (sy / sx);
    return deltaNdc;
}

float WE_SunDiskMask(float3 viewDir, float3 sunDir, float2 pixelNdc, out float2 sunNdc)
{
    sunNdc = WE_DirectionToNdc(sunDir);
    if (any(abs(sunNdc) > 1.5))
        return 0.0;

    // Only draw when the sun is in front of the camera.
    float4 viewSpace = mul(view, float4(sunDir, 0.0));
    if (viewSpace.z >= 0.0)
        return 0.0;

    const float angularRadius = max(sunAngularRadius, 0.004675);
    // Angular radius → aspect-corrected NDC radius via vertical focal length.
    const float ndcRadius = abs(tan(angularRadius) * abs(proj[1][1]));
    const float coronaScale = 4.5;
    const float coronaRadius = ndcRadius * coronaScale;

    float2 d = WE_AspectCorrectNdcDelta(pixelNdc - sunNdc);
    const float r = length(d);

    const float disk = 1.0 - smoothstep(ndcRadius * 0.85, ndcRadius * 1.05, r);
    const float coronaT = 1.0 - saturate((r - ndcRadius) / max(coronaRadius - ndcRadius, 1e-5));
    const float corona = coronaT * coronaT * (3.0 - 2.0 * coronaT);
    return saturate(disk + corona * 0.35);
}

float3 EncodeDisplay(float3 linearColor, int debugMode)
{
    if (debugMode == 5)
        return saturate(linearColor);

    float3 compressed = linearColor / (1.0 + linearColor * 0.35);
    return WE_LinearToSRGB(compressed);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 dir = normalize(input.viewDir);
    int debugMode = (int)round(cameraPadding);

    // Travel direction from the same EnvironmentBuffer used by clouds and PBR.
    const float3 sunDir = normalize(-sunDirection);
    const float3 sunTint = max(sunColor, float3(0.85, 0.85, 0.85));

    float3 sky = SkyGradient(dir);

    float2 sunNdc;
    float sunMask = 0.0;
    if (enableSunDisk > 0.5 && sunDir.y > -0.05)
        sunMask = WE_SunDiskMask(dir, sunDir, input.ndc, sunNdc);

    float3 sun = sunTint * sunMask * max(sunIntensity, 0.25) * 2.2;
    if (debugMode == 4)
        sun = 0.0;

    float3 linearColor = sky + sun;

    if (debugMode == 1)
        return float4(EncodeDisplay(sky, 0), 1.0);
    if (debugMode == 2)
        return float4(sunMask.xxx, 1.0);
    if (debugMode == 3)
    {
        float lum = dot(linearColor, float3(0.2126, 0.7152, 0.0722));
        float3 heat = lerp(float3(0.0, 0.0, 0.3), float3(1.0, 0.9, 0.1), saturate(lum));
        heat = lerp(heat, float3(1.0, 0.2, 0.1), saturate(lum - 1.0));
        return float4(heat, 1.0);
    }
    if (debugMode == 6)
        return float4(sunDir * 0.5 + 0.5, 1.0);
    if (debugMode == 7)
    {
        float2 d = WE_AspectCorrectNdcDelta(input.ndc - sunNdc);
        float cross = step(length(d), 0.01) + step(abs(d.x), 0.002) * step(abs(d.y), 0.04)
                    + step(abs(d.y), 0.002) * step(abs(d.x), 0.04);
        return float4(cross, sunMask, 0.0, 1.0);
    }
    if (debugMode == 8)
    {
        float2 d = WE_AspectCorrectNdcDelta(input.ndc - sunNdc);
        float r = length(d);
        return float4(frac(r * 40.0).xxx, 1.0);
    }

    return float4(EncodeDisplay(linearColor, debugMode), 1.0);
}

#endif
