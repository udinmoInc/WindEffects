#ifndef WE_BLOOM_HLSLI
#define WE_BLOOM_HLSLI

#include "../Common/Color.hlsli"

float3 WE_ExtractBloom(float3 hdrColor, float threshold)
{
    float luminance = dot(hdrColor, float3(0.2126, 0.7152, 0.0722));
    return hdrColor * saturate(luminance - threshold);
}

#endif // WE_BLOOM_HLSLI
