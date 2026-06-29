#ifndef WE_VOLUMETRIC_FOG_HLSLI
#define WE_VOLUMETRIC_FOG_HLSLI

#if defined(WE_VOLUMETRIC_FOG)
float3 WE_ApplyVolumetricFog(float3 color, float3 worldPos, float density)
{
    float fog = 1.0 - exp(-density * length(worldPos) * 0.01);
    return lerp(color, float3(0.6, 0.65, 0.7), fog);
}
#endif

#endif // WE_VOLUMETRIC_FOG_HLSLI
