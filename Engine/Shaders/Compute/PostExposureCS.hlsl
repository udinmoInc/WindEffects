#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

RWTexture2D<float4> sceneColor : register(u0, space1);

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint width = 0;
    uint height = 0;
    sceneColor.GetDimensions(width, height);
    if (dtid.x >= width || dtid.y >= height)
        return;

    float3 color = sceneColor[dtid.xy].rgb;
    color = WE_SanitizeHdrColor(color);
    // exposureEV already includes auto-exposure and compensation from CPU — apply once.
    const float exposureScale = WE_ExposureFromEV100(exposureEV);
    color = WE_ApplyFilmicTonemap(color, exposureScale);
    color = WE_LinearToSRGB(color);
    sceneColor[dtid.xy] = float4(color, 1.0);
}
