#ifndef WE_VOLUMETRIC_SHADOW_HLSLI
#define WE_VOLUMETRIC_SHADOW_HLSLI

// Shared volumetric light-transmittance / shadow framework.
// Quality is driven by VolumetricFrameUniform.shadowSteps / shadowQuality.

float WE_VolumetricShadowStepCount(uint shadowSteps, uint shadowQuality)
{
    // Quality tiers scale the configured step budget.
    const float q = (shadowQuality <= 1u) ? 0.5 :
                    (shadowQuality == 2u) ? 0.75 :
                    (shadowQuality >= 4u) ? 1.25 : 1.0;
    return max(float(shadowSteps) * q, 1.0);
}

float WE_LightTransmittanceFromOptical(float opticalDepth, float absorption)
{
    return exp(-opticalDepth * max(absorption, 1e-5));
}

#endif // WE_VOLUMETRIC_SHADOW_HLSLI
