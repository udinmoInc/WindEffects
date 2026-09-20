#ifndef WE_LOCAL_FOG_DENSITY_HLSLI
#define WE_LOCAL_FOG_DENSITY_HLSLI

#include "../VolumeSample.hlsli"

// Axis-aligned box local fog (provider-side).

VolumeSample EvaluateLocalFogSample(
    float3 worldPos,
    float3 center,
    float3 halfExtents,
    float density,
    float anisotropy,
    float enabled)
{
    if (enabled < 0.5 || density <= 1e-8)
        return EmptyVolumeSample();

    const float3 local = abs(worldPos - center);
    const float3 he = max(halfExtents, float3(0.01, 0.01, 0.01));
    // Soft edge falloff near box boundary.
    const float3 n = local / he;
    const float inside = 1.0 - saturate(max(n.x, max(n.y, n.z)));
    if (inside <= 1e-4)
        return EmptyVolumeSample();

    const float d = density * inside * inside;
    return MakeVolumeSample(d, d, d * 0.85, anisotropy);
}

#endif // WE_LOCAL_FOG_DENSITY_HLSLI
