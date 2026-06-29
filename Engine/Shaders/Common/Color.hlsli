#ifndef WE_COLOR_HLSLI
#define WE_COLOR_HLSLI

#include "Platform.hlsli"

float WE_NeutralGray(float3 c) { return (c.r + c.g + c.b) / 3.0; }

float3 WE_ToNeutral(float3 c)
{
    float g = WE_NeutralGray(c);
    return float3(g, g, g);
}

float WE_LogLerp(float a, float b, float t)
{
    float la = log(max(a, 1e-5));
    float lb = log(max(b, 1e-5));
    return exp(lerp(la, lb, saturate(t)));
}

// Display-space (gamma) output for R8G8B8A8_UNORM targets.
float3 WE_ClampCharcoalExposure(float3 color, float ceiling, float floorValue)
{
    float g = WE_NeutralGray(color);
    g = clamp(g, floorValue, ceiling);
    return float3(g, g, g);
}

#endif // WE_COLOR_HLSLI
