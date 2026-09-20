#ifndef WE_SHADOW_BUFFER_HLSLI
#define WE_SHADOW_BUFFER_HLSLI

// Must match we::runtime::renderer::ShadowCascadeUniform (std140).

#ifndef WE_SHADOW_BUFFER_REGISTER
#define WE_SHADOW_BUFFER_REGISTER b3
#endif

#ifndef WE_SHADOW_BUFFER_SPACE
#define WE_SHADOW_BUFFER_SPACE space0
#endif

cbuffer ShadowCascadeBuffer : register(WE_SHADOW_BUFFER_REGISTER, WE_SHADOW_BUFFER_SPACE)
{
    float4x4 shadowLightViewProj[4];
    float4   shadowCascadeSplits;       // xyzw = split distances
    float4   shadowAtlasScaleBias[4];   // xy scale, zw offset
    float4   shadowSunTravel;           // xyz travel, w castsShadows
    float4   shadowParams0;             // depthBias, normalBias, filterRadius, cascadeCount
    float4   shadowParams1;             // cascadeBlend, softShadows, contactShadows, contactLength
    float4   shadowParams2;             // atlasRes, cascadeRes, enabled, shadowDistance
};

#endif // WE_SHADOW_BUFFER_HLSLI
