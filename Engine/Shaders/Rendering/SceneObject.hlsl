#include "../Common/Camera.hlsli"
#include "../Common/Color.hlsli"

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position   : SV_Position;
    float3 worldPos   : TEXCOORD0;
    float3 worldNormal: TEXCOORD1;
    float2 texCoord   : TEXCOORD2;
};

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

cbuffer ObjectBuffer : register(b1, space0)
{
    float4x4 model;
    float4   color;
    int      mode;
    int3     objectPadding;
};

cbuffer EnvironmentBuffer : register(b2, space0)
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

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    o.worldPos = worldPos.xyz;
    o.worldNormal = normalize(mul(model, float4(input.normal, 0.0)).xyz);
    o.texCoord = input.texCoord;
    o.position = mul(proj, mul(view, worldPos));
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float3 albedo = WE_sRGBToLinear(saturate(color.rgb));

    if (mode == 1 || mode == 2)
    {
        const float3 mapped = WE_ApplyFilmicTonemap(albedo, WE_ExposureFromEV100(1.85));
        return float4(WE_LinearToSRGB(mapped), color.a);
    }

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(sunDirection);
    float3 viewDir = normalize(cameraPos - input.worldPos);

    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));
    const float3 skyUpper = WE_sRGBToLinear(saturate(skyAmbientColor));
    const float3 skyLower = WE_sRGBToLinear(saturate(skyLightLowerColor));
    const float upN = saturate(normal.y * 0.5 + 0.5);
    const float3 ambient = lerp(skyLower * 0.06, skyUpper * 0.14, upN) * skyLightIntensity;

    float3 lightDir = normalize(sunDirection);
    float3 viewDir = normalize((cameraPos - worldOrigin) - (input.worldPos - worldOrigin));
    const float fogAmount = enableVolumetricFog > 0.5
        ? (1.0 - exp(-fogDensity * height * fogHeightFalloff))
        : 0.0;
    const float distToCamera = length(cameraPos - input.worldPos);
    const float distFog = 1.0 - exp(-distToCamera * fogDensity * 0.35);
    const float3 fogLinear = WE_sRGBToLinear(saturate(fogColor));
    litLinear = lerp(litLinear, fogLinear, saturate(fogAmount * 0.65 + distFog * 0.35));

    const float haze = 1.0 - exp(-distToCamera * 0.0035);
    const float3 aerial = WE_sRGBToLinear(saturate(aerialTint)) * atmosphereRayleigh * 120.0;
    litLinear = lerp(litLinear, aerial, saturate(haze * 0.55));

    const float3 mapped = WE_ApplyFilmicTonemap(litLinear, WE_ExposureFromEV100(1.85));
    return float4(WE_LinearToSRGB(mapped), color.a);
}
