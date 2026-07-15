#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/CloudFrameBuffer.hlsli"
#include "VolumetricClouds.hlsli"
#include "../Common/CameraBuffer.hlsli"

// Half-res cloud raymarch → linear RGB + opacity.
// Debug: 10 white, 11 bounds, 12 altitude planes, 13 weather, 14 shape, 15 density,
// 16 steps, 28 entry/exit, 29 march distance, 30 sample positions, 31 cloud mask,
// 19 ray origin, 20 ray direction, 21 empty skips.

Texture2D<float> sceneDepth : register(t4, space2);
SamplerState cloudSharedSampler : register(s3, space2);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float4 clip = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    VSOutput o;
    o.position = float4(clip.xy, 0.0, 1.0);
    o.uv = uv * 0.5;
    return o;
}

// Convert hardware depth (perspectiveRH_ZO: near→0, far→1) to world-space ray distance.
float WE_SceneHitDistance(float2 uv)
{
    const float z = sceneDepth.SampleLevel(cloudSharedSampler, uv, 0).r;
    // Empty / far clear → no occlusion (PBR clears depth to 1.0).
    if (z >= 0.9999)
        return 1e8;

    const float2 ndc = uv * 2.0 - 1.0;
    const float3 worldPos = WE_UnprojectPoint(ndc.x, ndc.y, z, view, proj);
    return max(length(worldPos - cameraPos), 0.0);
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableClouds < 0.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    const float2 uv = saturate(input.uv + cloudTemporalJitter * cloudInvCloudResolution);
    const float3 viewDir = WE_UnprojectDirection(uv, view, proj, cameraPos);
    const float3 sunDir = normalize(-sunDirection);
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    const int cloudDebug = atmosphereDebugMode;
    const float sceneT = WE_SceneHitDistance(uv);

    if (cloudDebug == 19)
    {
        const float3 o = cameraPos * 0.001 + 0.5;
        return float4(saturate(o), 1.0);
    }
    if (cloudDebug == 20)
        return float4(viewDir * 0.5 + 0.5, 1.0);

    float t0 = 0.0;
    float t1 = 0.0;
    const bool hitVolume = WE_IntersectCloudVolume(cameraPos, viewDir, worldOrigin, t0, t1);

    // No intersection → write fully transparent (critical: never a fullscreen veil).
    if (!hitVolume || sceneT <= t0)
        return float4(0.0, 0.0, 0.0, 0.0);

    t1 = min(t1, sceneT);
    if (t1 <= t0)
        return float4(0.0, 0.0, 0.0, 0.0);

    if (cloudDebug == 10)
        return float4(0.92, 0.94, 0.98, 0.75);

    if (cloudDebug == 11)
    {
        // Bounding volume viz — cyan interior of hit segment only.
        const float midT = 0.5 * (t0 + t1);
        const float3 hit = cameraPos + viewDir * midT;
        const float radial = WE_CloudRadialMask(hit, worldOrigin);
        const float h = saturate((hit.y - (worldOrigin.y + cloudBottomAltitude))
            / max(cloudTopAltitude - cloudBottomAltitude, 1.0));
        return float4(0.1 * radial, 0.55 + h * 0.4, 0.85, 0.55 * radial);
    }

    if (cloudDebug == 12)
    {
        // Bottom/top altitude plane hit masks.
        const float bottom = worldOrigin.y + cloudBottomAltitude;
        const float top = worldOrigin.y + cloudTopAltitude;
        const float3 p0 = cameraPos + viewDir * t0;
        const float nearBottom = 1.0 - saturate(abs(p0.y - bottom) / 40.0);
        const float3 p1 = cameraPos + viewDir * t1;
        const float nearTop = 1.0 - saturate(abs(p1.y - top) / 40.0);
        return float4(nearBottom, 0.15, nearTop, saturate(max(nearBottom, nearTop) + 0.2));
    }

    if (cloudDebug == 28)
    {
        // Entry (R) / exit (G) normalized distances.
        const float te = saturate(t0 / max(cloudDomainRadius, 1.0));
        const float tx = saturate(t1 / max(cloudDomainRadius, 1.0));
        return float4(te, tx, 0.0, 0.85);
    }

    if (cloudDebug == 29)
    {
        const float len = saturate((t1 - t0) / max(cloudThickness * 2.0, 1.0));
        return float4(len, len * 0.5, 1.0 - len, 0.85);
    }

    float3 samplePos = cameraPos + viewDir * (0.5 * (t0 + t1));

    if (cloudDebug == 13)
    {
        const float w = WE_CloudWeatherCoverage(samplePos) * WE_CloudRadialMask(samplePos, worldOrigin);
        return float4(w, w * 0.85, w * 0.55, saturate(w + 0.05));
    }
    if (cloudDebug == 14)
    {
        const float s = WE_CloudShapeNoiseSample(samplePos);
        return float4(s, s * 0.7, s * 0.4, 0.9);
    }
    if (cloudDebug == 15)
    {
        const float d = saturate(WE_CloudDensityAt(samplePos, worldOrigin) * 1.5);
        return float4(d, d * 0.5, d * 0.2, saturate(d + 0.05));
    }
    if (cloudDebug == 30)
    {
        // World-space sample position (XZ / height).
        const float3 p = samplePos * 0.00008 + 0.5;
        return float4(saturate(p.x), saturate((samplePos.y - cloudBottomAltitude) / max(cloudThickness, 1.0)), saturate(p.z), 0.9);
    }

    float opacity = 0.0;
    float avgDensity = 0.0;
    float stepsTaken = 0.0;
    float emptySkips = 0.0;
    float tEnter = 0.0;
    float tExit = 0.0;
    const float3 cloudsLinear = WE_RaymarchClouds(
        cameraPos, viewDir, worldOrigin,
        sunDir, sunLinear, sunIntensity,
        uv, sceneT,
        opacity, avgDensity, stepsTaken, emptySkips,
        tEnter, tExit);

    if (cloudDebug == 16)
    {
        const float s = saturate(stepsTaken / max(float(cloudQualitySteps), 1.0));
        return float4(s, 1.0 - s, avgDensity, saturate(opacity + 0.05));
    }
    if (cloudDebug == 21)
    {
        const float e = saturate(emptySkips / max(stepsTaken, 1.0));
        return float4(e, 0.2, 1.0 - e, 0.85);
    }
    if (cloudDebug == 31)
    {
        // Final cloud mask (alpha only).
        return float4(opacity, opacity, opacity, saturate(opacity + 0.02));
    }

    if (opacity <= 1e-4)
        return float4(0.0, 0.0, 0.0, 0.0);

    return float4(max(cloudsLinear, 0.0), saturate(opacity));
}
