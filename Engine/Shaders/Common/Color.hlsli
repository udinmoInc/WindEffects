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

float3 WE_sRGBToLinear(float3 c)
{
    const float3 lo = c / 12.92;
    const float3 hi = pow(max((c + 0.055) / 1.055, 0.0), 2.4);
    return lerp(hi, lo, step(c, 0.04045));
}

float3 WE_LinearToSRGB(float3 c)
{
    c = max(c, 0.0);
    const float3 lo = c * 12.92;
    const float3 hi = 1.055 * pow(max(c, 1e-8), 1.0 / 2.4) - 0.055;
    return saturate(lerp(hi, lo, step(c, 0.0031308)));
}

float3 WE_ACESFilm(float3 x)
{
    // Narkowicz ACES approximation.
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float WE_ExposureFromEV100(float ev100)
{
    // EV100 to exposure scale used in physically based camera models.
    return 1.0 / max(pow(2.0, ev100), 1e-5);
}

float3 WE_ApplyFilmicTonemap(float3 linearColor, float exposureScale)
{
    const float3 exposed = max(linearColor * max(exposureScale, 1e-4), 0.0);
    // Slight shoulder before ACES keeps sky gradients and bright clouds from clipping to flat cyan.
    const float3 compressed = exposed / (1.0 + exposed * 0.18);
    return WE_ACESFilm(compressed);
}

float3 WE_SanitizeHdrColor(float3 color)
{
    if (any(isnan(color)) || any(isinf(color)))
        return float3(0.0, 0.0, 0.0);
    return clamp(color, float3(0.0, 0.0, 0.0), float3(65504.0, 65504.0, 65504.0));
}

float WE_ComputeExposureEV(float3 lightTravelDir, float baseEV, float compensation)
{
    const float sunElevation = normalize(-lightTravelDir).y;
    const float dayFactor = saturate(sunElevation * 2.5 + 0.1);
    const float twilightFactor = saturate(1.0 - abs(sunElevation) * 3.0);
    const float nightEV = baseEV - 5.0;
    const float dayEV = baseEV + 1.0;
    const float twilightEV = baseEV - 1.5;
    float ev = lerp(nightEV, dayEV, dayFactor);
    ev = lerp(ev, twilightEV, twilightFactor * (1.0 - dayFactor));
    return ev + compensation;
}

// Display-space clamp helper for ultra-dark backgrounds.
float3 WE_ClampCharcoalExposure(float3 color, float ceiling, float floorValue)
{
    float g = WE_NeutralGray(color);
    g = clamp(g, floorValue, ceiling);
    return float3(g, g, g);
}

#endif // WE_COLOR_HLSLI
