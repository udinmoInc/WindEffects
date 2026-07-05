#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

Texture2D<float4> sceneColor : register(t0, space1);
Texture2D<float4> bloomTexture : register(t1, space1);
Texture2D<float> luminanceAvg : register(t2, space1);
RWTexture2D<float4> sceneOutput : register(u0, space2);
SamplerState linearSampler : register(s0, space1);

float WE_ComputeAutoExposureEV(float avgLuminance)
{
    const float middleGray = 0.18;
    const float clamped = max(avgLuminance, 1e-5);
    // EV so that 1/2^EV = middleGray/avgLuminance (normalize scene average to key).
    return log2(clamped / middleGray);
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

    const float2 bloomUv = (float2(dtid.xy) + 0.5) / float2(width, height);
    const float3 bloom = bloomTexture.SampleLevel(linearSampler, bloomUv, 0.0).rgb;
    color += bloom * bloomIntensity;

    const float avgLum = max(luminanceAvg.Load(int3(0, 0, 0)), 1e-6);
    const float autoEV = clamp(
        WE_ComputeAutoExposureEV(avgLum) + exposureCompensation,
        -4.0, 8.0);
    const float ev = lerp(exposureEV, autoEV, saturate(enableAutoExposure));

    const float exposureScale = WE_ExposureFromEV100(ev);
    color = WE_ApplyFilmicTonemap(color, exposureScale);
    color = WE_LinearToSRGB(color);
    sceneOutput[dtid.xy] = float4(color, 1.0);
}
