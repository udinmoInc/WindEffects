#ifndef WE_DEFERRED_HLSLI
#define WE_DEFERRED_HLSLI

#include "Lighting.hlsli"

#if defined(WE_DEFERRED)
struct WE_GBufferOutput
{
    float4 albedoMetallic : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 emissive : SV_Target2;
};
#endif

#endif // WE_DEFERRED_HLSLI
