#include "../Common/Color.hlsli"

Texture2D<float4> sceneColor : register(t0, space1);
RWTexture2D<float> luminanceTiles : register(u0, space2);

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint sceneW = 0;
    uint sceneH = 0;
    sceneColor.GetDimensions(sceneW, sceneH);

    uint tileW = 0;
    uint tileH = 0;
    luminanceTiles.GetDimensions(tileW, tileH);
    if (dtid.x >= tileW || dtid.y >= tileH)
        return;

    const uint blockW = max((sceneW + tileW - 1) / tileW, 1u);
    const uint blockH = max((sceneH + tileH - 1) / tileH, 1u);
    const uint2 start = dtid.xy * uint2(blockW, blockH);

    float sum = 0.0;
    uint count = 0u;
    [loop]
    for (uint y = 0; y < blockH; ++y)
    {
        [loop]
        for (uint x = 0; x < blockW; ++x)
        {
            const uint2 coord = min(start + uint2(x, y), uint2(sceneW - 1, sceneH - 1));
            const float3 hdr = WE_SanitizeHdrColor(sceneColor[coord].rgb);
            sum += dot(hdr, float3(0.2126, 0.7152, 0.0722));
            count++;
        }
    }

    luminanceTiles[dtid.xy] = sum / max(float(count), 1.0);
}
