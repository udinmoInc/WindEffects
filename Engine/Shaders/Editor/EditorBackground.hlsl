#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/Noise.hlsli"

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

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

// UE5 empty-editor backdrop — almost pure black, no visible gray horizon.
static const float kBlackTop        =  9.0 / 255.0; // #090909
static const float kBlackBottom     = 14.0 / 255.0; // #0E0E0E
static const float kMaxGradientSpan = 0.014;          // ~1.4 % luminance span
static const float kExposureCeiling = 15.0 / 255.0;
static const float kExposureFloor   =  8.0 / 255.0;

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
    float screenV = saturate(input.uv.y);

    float uTop = WE_NeutralGray(WE_ToNeutral(zenithColor));
    float uBot = WE_NeutralGray(WE_ToNeutral(bottomColor));
    float anchor = (kBlackTop + kBlackBottom) * 0.5;

    float mid = lerp(anchor, (uTop + uBot) * 0.5, saturate(gradientStrength) * 0.6);
    float halfSpan = min(abs(uBot - uTop) * 0.5, mid * kMaxGradientSpan * 0.5);

    float gTop = mid - halfSpan;
    float gBot = mid + halfSpan;

    gTop = max(gTop, kExposureFloor);
    gBot = min(gBot, kExposureCeiling);
    gBot = max(gBot, gTop + mid * 0.004);

    // Gentle vertical falloff — no atmospheric fog or bright horizon band.
    float strength = max(0.20, saturate(gradientStrength) * 0.5);
    float expK = lerp(1.2, 1.8, strength);
    float t = (1.0 - exp(-screenV * expK)) / (1.0 - exp(-expK));
    float lum = lerp(gTop, gBot, t);

    lum *= saturate(backgroundBrightness);
    lum = clamp(lum, kExposureFloor, kExposureCeiling);

    float3 color = float3(lum, lum, lum);

    float2 pixel = input.position.xy;
    float dither = (WE_BlueNoise(pixel) + WE_InterleavedGradientNoise(pixel)) * 0.5 - 0.5;
    color += dither / 255.0;

    color = WE_ClampCharcoalExposure(color, kExposureCeiling, kExposureFloor);
    return float4(color, 1.0);
}
