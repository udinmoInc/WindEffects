Texture2D<float> luminanceTiles : register(t0, space1);
RWTexture2D<float> luminanceAvg : register(u0, space2);

[numthreads(1, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint tileW = 0;
    uint tileH = 0;
    luminanceTiles.GetDimensions(tileW, tileH);

    float sum = 0.0;
    [loop]
    for (uint y = 0; y < tileH; ++y)
    {
        [loop]
        for (uint x = 0; x < tileW; ++x)
            sum += luminanceTiles[int2(x, y)];
    }

    luminanceAvg[int2(0, 0)] = sum / max(float(tileW * tileH), 1.0);
}
