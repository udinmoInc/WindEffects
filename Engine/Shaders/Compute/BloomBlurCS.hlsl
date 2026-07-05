Texture2D<float4> bloomSource : register(t0, space1);
RWTexture2D<float4> bloomTarget : register(u0, space2);

cbuffer BloomBlurParams : register(b0, space0)
{
    float2 blurDirection;
    float2 blurPadding;
};

static const float kKernel[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint width = 0;
    uint height = 0;
    bloomTarget.GetDimensions(width, height);
    if (dtid.x >= width || dtid.y >= height)
        return;

    float3 accum = float3(0.0, 0.0, 0.0);
    [unroll]
    for (int i = -2; i <= 2; ++i)
    {
        const float2 offset = blurDirection * float(i);
        const int2 coord = int2(dtid.xy) + int2(offset);
        const int2 clamped = int2(
            clamp(coord.x, 0, int(width) - 1),
            clamp(coord.y, 0, int(height) - 1));
        accum += bloomSource[clamped].rgb * kKernel[i + 2];
    }

    bloomTarget[dtid.xy] = float4(max(accum, 0.0), 1.0);
}
