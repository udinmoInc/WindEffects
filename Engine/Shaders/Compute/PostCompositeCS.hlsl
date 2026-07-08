#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

Texture2D<float4> sceneColor : register(t0, space1);
Texture2D<float4> bloomTexture : register(t1, space1);
Texture2D<float> luminanceAvg : register(t2, space1);
RWTexture2D<float4> sceneOutput : register(u0, space2);
SamplerState linearSampler : register(s0, space1);

float WE_ComputeAutoExposureEV(float avgLuminance, float skyLumHint, float bloomWeight)
{
    const float middleGray = 0.18;
    // GPU average ignores bloom energy added later; hdrSkyLuminance stabilizes outdoor key.
    const float effectiveAvg = max(max(avgLuminance, skyLumHint * 0.35), 1e-5);
    const float bloomHeadroom = 1.0 + bloomWeight * 0.85;
    return log2(effectiveAvg * bloomHeadroom / middleGray);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint width = 0;
    uint height = 0;
    sceneOutput.GetDimensions(width, height);
    if (dtid.x >= width || dtid.y >= height)
        return;

    float3 color = WE_SanitizeHdrColor(sceneColor[dtid.xy].rgb);

    if (pipelineBypassToneMapping != 0)
    {
        const float evScale = max(WE_ExposureFromEV100(exposureEV), 1.0 / 4096.0);
        const float exposureScale = pipelineFixedExposureMultiplier > 0.0
            ? pipelineFixedExposureMultiplier
            : evScale;
        color = max(color * exposureScale, 0.0);
        sceneOutput[dtid.xy] = float4(color, 1.0);
        return;
    }

    const float2 bloomUv = (float2(dtid.xy) + 0.5) / float2(width, height);
    const float3 bloom = bloomTexture.SampleLevel(linearSampler, bloomUv, 0.0).rgb;
    color += bloom * bloomIntensity;

    const float avgLum = max(luminanceAvg.Load(int3(0, 0, 0)), 1e-6);
    const float autoEV = clamp(
        WE_ComputeAutoExposureEV(avgLum, hdrSkyLuminance, bloomIntensity) + exposureCompensation,
        -2.0, 14.0);
    const float ev = lerp(exposureEV, autoEV, saturate(enableAutoExposure));

    const float evScale = max(WE_ExposureFromEV100(ev), 1.0 / 4096.0);
    const float exposureScale = (enableAutoExposure < 0.5 && pipelineFixedExposureMultiplier > 0.0)
        ? pipelineFixedExposureMultiplier
        : evScale;
    color = WE_ApplyFilmicTonemap(color, exposureScale);
    color = WE_LinearToSRGB(color);
    sceneOutput[dtid.xy] = float4(color, 1.0);
}
