#ifndef WE_SHADOW_HLSLI
#define WE_SHADOW_HLSLI

// Cascaded shadow sampling for the Sun directional light.
// Include ShadowBuffer.hlsli before this file when WE_SUN_SHADOWS_ENABLED == 1.

#include "../Common/Math.hlsli"

#ifndef WE_SUN_SHADOWS_ENABLED
#define WE_SUN_SHADOWS_ENABLED 0
#endif

#if WE_SUN_SHADOWS_ENABLED
Texture2D WE_SunShadowAtlas : register(t4, space0);
SamplerComparisonState WE_SunShadowCompareSampler : register(s4, space0);

float WE_ShadowCascadeIndex(float viewDepth, float4 splits, float cascadeCount)
{
    float idx = 0.0;
    idx += viewDepth > splits.x ? 1.0 : 0.0;
    idx += viewDepth > splits.y ? 1.0 : 0.0;
    idx += viewDepth > splits.z ? 1.0 : 0.0;
    return min(idx, max(cascadeCount - 1.0, 0.0));
}

float3 WE_ShadowWorldToAtlasUvDepth(float3 worldPos, int cascade)
{
    float4 clip = mul(shadowLightViewProj[cascade], float4(worldPos, 1.0));
    float3 ndc = clip.xyz / max(abs(clip.w), 1e-6);
    float2 uv = ndc.xy * 0.5 + 0.5;
    float2 atlasUv = uv * shadowAtlasScaleBias[cascade].xy + shadowAtlasScaleBias[cascade].zw;
    return float3(atlasUv, saturate(ndc.z));
}

float WE_SampleShadowMapPCF(float3 atlasUvDepth, float filterRadiusTexels, float atlasRes)
{
    const float2 texel = filterRadiusTexels / max(atlasRes, 1.0);
    float sum = 0.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            sum += WE_SunShadowAtlas.SampleCmpLevelZero(
                WE_SunShadowCompareSampler,
                atlasUvDepth.xy + float2(x, y) * texel,
                atlasUvDepth.z);
        }
    }
    return sum * (1.0 / 9.0);
}

float WE_SampleSunShadowCSM(float3 worldPos, float3 worldNormal, float viewDepth)
{
    if (shadowParams2.z < 0.5 || shadowSunTravel.w < 0.5)
        return 1.0;

    const float cascadeCount = max(shadowParams0.w, 1.0);
    const float3 biasedPos = worldPos + normalize(worldNormal) * shadowParams0.y;

    const float cascade = WE_ShadowCascadeIndex(viewDepth, shadowCascadeSplits, cascadeCount);
    const int c0 = (int)cascade;
    const int c1 = min(c0 + 1, (int)cascadeCount - 1);

    float3 uv0 = WE_ShadowWorldToAtlasUvDepth(biasedPos, c0);
    uv0.z = saturate(uv0.z + shadowParams0.x);

    float shadow0 = (shadowParams1.y > 0.5)
        ? WE_SampleShadowMapPCF(uv0, shadowParams0.z, shadowParams2.x)
        : WE_SunShadowAtlas.SampleCmpLevelZero(WE_SunShadowCompareSampler, uv0.xy, uv0.z);

    // Blend near the far edge of cascade c0 into c1.
    float splitFar = shadowCascadeSplits[c0];
    float splitNear = (c0 == 0) ? 0.0 : shadowCascadeSplits[c0 - 1];
    float range = max(splitFar - splitNear, 1e-3);
    float blendWidth = max(range * shadowParams1.x, 1e-3);
    float blend = saturate((viewDepth - (splitFar - blendWidth)) / blendWidth);

    if (blend <= 0.001 || c0 == c1)
        return saturate(shadow0);

    float3 uv1 = WE_ShadowWorldToAtlasUvDepth(biasedPos, c1);
    uv1.z = saturate(uv1.z + shadowParams0.x);
    float shadow1 = (shadowParams1.y > 0.5)
        ? WE_SampleShadowMapPCF(uv1, shadowParams0.z, shadowParams2.x)
        : WE_SunShadowAtlas.SampleCmpLevelZero(WE_SunShadowCompareSampler, uv1.xy, uv1.z);
    return saturate(lerp(shadow0, shadow1, blend));
}

/// Optional contact shadow supplement (requires scene depth + viewProj). Returns 1 if disabled.
float WE_SampleSunContactShadow(
    float3 worldPos,
    float3 worldNormal,
    float4x4 viewProj,
    Texture2D sceneDepthTex,
    SamplerState sceneDepthSampler,
    float2 viewportSize)
{
    if (shadowParams1.z < 0.5 || shadowParams1.w <= 1e-4)
        return 1.0;

    const float3 toSun = normalize(-shadowSunTravel.xyz);
    if (dot(worldNormal, toSun) <= 0.02)
        return 1.0;

    const int steps = 8;
    const float len = shadowParams1.w;
    float occ = 0.0;
    [loop]
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float(i) / float(steps)) * len;
        const float3 sampleWorld = worldPos + worldNormal * 0.02 + toSun * t;
        float4 clip = mul(viewProj, float4(sampleWorld, 1.0));
        if (clip.w <= 1e-5)
            continue;
        float3 ndc = clip.xyz / clip.w;
        float2 uv = ndc.xy * 0.5 + 0.5;
        if (any(uv < 0.0) || any(uv > 1.0))
            continue;
        float sceneZ = sceneDepthTex.SampleLevel(sceneDepthSampler, uv, 0).r;
        // ZO depth: smaller z is closer. Occluder if scene is closer than sample.
        if (sceneZ + 1e-4 < ndc.z)
            occ += 1.0;
    }
    (void)viewportSize;
    return 1.0 - saturate(occ / float(steps));
}
#endif // WE_SUN_SHADOWS_ENABLED

float WE_EvaluateSunVisibility(float3 worldPos, float3 worldNormal, float viewDepth)
{
#if WE_SUN_SHADOWS_ENABLED
    return WE_SampleSunShadowCSM(worldPos, worldNormal, viewDepth);
#else
    (void)worldPos;
    (void)worldNormal;
    (void)viewDepth;
    return 1.0;
#endif
}

#endif // WE_SHADOW_HLSLI
