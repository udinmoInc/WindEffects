#ifndef WE_VOLUMETRIC_TEMPORAL_HLSLI
#define WE_VOLUMETRIC_TEMPORAL_HLSLI

// Shared temporal integration ownership (jitter / blend / history validation stubs).
// History textures are allocated by VolumetricRenderer; binding comes later.

float WE_VolumetricRayJitter(float2 pixel, float jitterX, float jitterY)
{
    const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    const float spatial = frac(magic.z * frac(dot(pixel, magic.xy)));
    return frac(spatial + jitterX * 0.37 + jitterY * 0.73);
}

float4 WE_VolumetricTemporalAccumulate(
    float4 current,
    float4 history,
    float temporalBlend,
    uint temporalQuality)
{
    if (temporalQuality == 0u)
        return current;

    const float blend = saturate(temporalBlend);
    // Basic history validation: reject wildly different alpha.
    const float alphaDelta = abs(current.a - history.a);
    const float valid = (alphaDelta < 0.45) ? 1.0 : 0.0;
    const float w = blend * valid;
    return lerp(current, history, w);
}

#endif // WE_VOLUMETRIC_TEMPORAL_HLSLI
