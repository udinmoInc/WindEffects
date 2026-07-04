#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/Noise.hlsli"
#include "../Rendering/SkyAtmosphere.hlsli"

cbuffer EnvironmentBuffer : register(b0, space0)
{
    float3 sunDirection;
    float  sunIntensity;
    float3 sunColor;
    float  skyLightIntensity;
    float3 skyAmbientColor;
    float  fogDensity;
    float3 skyLightLowerColor;
    float  fogHeightFalloff;
    float3 fogColor;
    float  fogStartDistance;
    float3 atmosphereRayleigh;
    float  mieScattering;
    float3 aerialTint;
    float  mieAnisotropy;
    float3 worldOrigin;
    float  exposureEV;
    float  planetRadius;
    float  atmosphereHeight;
    float  enableVolumetricFog;
    float  enableClouds;
    int    sunCastShadows;
    int    sunTemperature;
    int2   envPadding;
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
    const float2 screenUV = input.uv;
    const float3 viewDir = WE_UnprojectDirection(screenUV, view, proj);

    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));
    const float3 fogLinear = WE_sRGBToLinear(saturate(fogColor));
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));

    float3 skyLinear = WE_SampleSkyAtmosphere(
        viewDir,
        sunDirection,
        sunLinear,
        sunIntensity,
        rayleigh,
        mieScattering,
        mieAnisotropy,
        planetRadius,
        atmosphereHeight,
        fogLinear,
        fogDensity);

    // Aerial perspective tint toward horizon
    const float horizonBlend = pow(saturate(1.0 - abs(viewDir.y)), 3.0);
    skyLinear = lerp(skyLinear, WE_sRGBToLinear(saturate(aerialTint)) * 0.25, horizonBlend * 0.15);

    const float exposureScale = WE_ExposureFromEV100(exposureEV);
    float3 color = WE_ApplyFilmicTonemap(skyLinear, exposureScale);
    color = WE_LinearToSRGB(color);

    float2 pixel = input.position.xy;
    float dither = (WE_BlueNoise(pixel) + WE_InterleavedGradientNoise(pixel)) * 0.5 - 0.5;
    color += dither / 255.0;

    return float4(color, 1.0);
}
