#ifndef WE_HEIGHT_FOG_DENSITY_HLSLI
#define WE_HEIGHT_FOG_DENSITY_HLSLI

#include "../VolumeSample.hlsli"

// Exponential height fog density evaluation (provider-side).

VolumeSample EvaluateHeightFogSample(
    float3 worldPos,
    float3 cameraPos,
    float density,
    float heightFalloff,
    float startDistance,
    float enabled)
{
    if (enabled < 0.5 || density <= 1e-8)
        return EmptyVolumeSample();

    const float height = max(worldPos.y, 0.0);
    const float heightTerm = exp(-height * max(heightFalloff, 0.01));
    const float dist = length(worldPos - cameraPos);
    const float distTerm = saturate((dist - startDistance) / max(dist, 1.0));
    const float d = density * heightTerm * lerp(0.35, 1.0, distTerm);

    // Fog: extinction ≈ density, isotropic scattering.
    return MakeVolumeSample(d, d, d * 0.9, 0.0);
}

#endif // WE_HEIGHT_FOG_DENSITY_HLSLI
