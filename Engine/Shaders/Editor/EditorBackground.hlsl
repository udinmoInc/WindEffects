#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/Noise.hlsli"
#include "../Common/Camera.hlsli"

cbuffer BackgroundSettings : register(b0, space0)
{
    float3 zenithColor;
    float  backgroundBrightness;
    float3 upperSkyColor;
    float  gradientStrength;
    float3 midSkyColor;
    float  horizonFade;
    float3 horizonColor;
    float  padding0;
    float3 bottomColor;
    float  backgroundContrast;
};

cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

// Keep the viewport dark while preserving subtle depth cues.
static const float kDisplayBlackTop    = 9.0 / 255.0;
static const float kDisplayBlackBottom = 14.0 / 255.0;
static const float kDisplayCeiling     = 15.0 / 255.0;
static const float kDisplayFloor       = 8.0 / 255.0;

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 pos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    VSOutput o;
    o.position = float4(pos, 0.0, 1.0);
    o.uv = uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float screenV = saturate(input.uv.y);
    const float3 viewDir = WE_UnprojectDirection(input.uv, view, proj);
    const float upFactor = saturate(viewDir.y * 0.5 + 0.5);
    const float horizonFactor = 1.0 - abs(viewDir.y);

    const float3 topLinear = WE_sRGBToLinear(WE_ToNeutral(zenithColor));
    const float3 botLinear = WE_sRGBToLinear(WE_ToNeutral(bottomColor));
    const float3 anchorLinear = WE_sRGBToLinear(float3((kDisplayBlackTop + kDisplayBlackBottom) * 0.5, (kDisplayBlackTop + kDisplayBlackBottom) * 0.5, (kDisplayBlackTop + kDisplayBlackBottom) * 0.5));

    const float gradShape = smoothstep(0.0, 1.0, pow(screenV, 1.35));
    const float3 baseLinear = lerp(lerp(anchorLinear, botLinear, 0.35), lerp(anchorLinear, topLinear, 0.65), gradShape);

    // Very subtle atmospheric scattering near horizon for depth; stays in near-black range.
    const float atmosphereDensity = lerp(0.004, 0.011, saturate(gradientStrength));
    const float horizonOpticalDepth = pow(saturate(horizonFactor), 1.8) * lerp(0.35, 1.0, 1.0 - upFactor);
    const float transmittance = exp(-horizonOpticalDepth * 24.0 * atmosphereDensity);
    const float3 scatterTint = WE_sRGBToLinear(float3(11.0 / 255.0, 11.0 / 255.0, 11.0 / 255.0));
    const float3 linearColor = lerp(scatterTint, baseLinear, transmittance) * saturate(backgroundBrightness);

    // Conservative exposure to preserve deep blacks without crushing.
    const float exposureScale = WE_ExposureFromEV100(2.35);
    float3 color = WE_ApplyFilmicTonemap(linearColor, exposureScale);
    color = WE_LinearToSRGB(color);
    color = WE_ClampCharcoalExposure(color, kDisplayCeiling, kDisplayFloor);

    float2 pixel = input.position.xy;
    float dither = (WE_BlueNoise(pixel) + WE_InterleavedGradientNoise(pixel)) * 0.5 - 0.5;
    color += dither / 255.0;

    return float4(color, 1.0);
}
