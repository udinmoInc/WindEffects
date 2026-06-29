#ifndef WE_SSAO_HLSLI
#define WE_SSAO_HLSLI

#include "../Common/Noise.hlsli"

float WE_SSAOKernel(float2 uv, float depth, float radius)
{
    return saturate(1.0 - WE_BlueNoise(uv * 137.0) * radius * depth * 0.01);
}

#endif // WE_SSAO_HLSLI
