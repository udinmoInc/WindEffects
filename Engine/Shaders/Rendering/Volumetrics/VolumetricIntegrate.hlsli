#ifndef WE_VOLUMETRIC_INTEGRATE_HLSLI
#define WE_VOLUMETRIC_INTEGRATE_HLSLI

#include "VolumeSample.hlsli"
#include "VolumetricScattering.hlsli"

// Shared front-to-back volumetric integration step.

void WE_AccumulateVolumeStep(
    VolumeSample sample,
    float stepLength,
    float viewExtinctionCoeff,
    float3 lightingRGB,
    inout float3 accumulatedColor,
    inout float transmittance)
{
    if (sample.density <= 1e-5 || stepLength <= 0.0)
        return;

    const float3 stepScattering = lightingRGB * sample.scattering;
    const float extinctionCoeff = max(max(sample.extinction / max(sample.density, 1e-6), viewExtinctionCoeff), 1e-4);
    const float stepExtinction = WE_ViewTransmittance(sample.density, stepLength, extinctionCoeff);
    const float3 stepIntegral =
        stepScattering * ((1.0 - stepExtinction) / max(sample.density * extinctionCoeff, 1e-6));
    accumulatedColor += stepIntegral * transmittance;
    transmittance *= stepExtinction;
}

#endif // WE_VOLUMETRIC_INTEGRATE_HLSLI
