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

// UE5 empty-editor backdrop constants (display / gamma space).
static const float kCharcoalAnchor = 23.0 / 255.0; // #171717
static const float kMaxGradientSpan = 0.06;          // 6% top-to-bottom (within 5–8% spec)
static const float kExposureCeiling = 27.0 / 255.0; // hard cap — prevents wash-out
static const float kExposureFloor   = 21.0 / 255.0; // hard floor — stays very dark

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    // Single oversized triangle — no quad seams or rectangle edges.
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 pos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    VSOutput o;
    o.position = float4(pos, 0.0, 1.0);
    o.uv = uv;
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Screen-space gradient only — camera rotation cannot create a world horizon line.
    float screenV = saturate(input.uv.y);

    float uTop = WE_NeutralGray(WE_ToNeutral(zenithColor));
    float uBot = WE_NeutralGray(WE_ToNeutral(bottomColor));
    float anchor = WE_NeutralGray(WE_ToNeutral(float3(kCharcoalAnchor, kCharcoalAnchor, kCharcoalAnchor)));

    float mid = lerp(anchor, (uTop + uBot) * 0.5, saturate(gradientStrength));
    float halfSpan = min(abs(uTop - uBot) * 0.5, mid * kMaxGradientSpan * 0.5);

    float gTop = mid + halfSpan;
    float gBot = mid - halfSpan;

    gTop = min(gTop, kExposureCeiling);
    gBot = max(gBot, kExposureFloor);
    gTop = max(gTop, gBot + mid * 0.01);

    // Smooth exponential atmospheric falloff toward bottom of viewport.
    float strength = max(0.35, saturate(gradientStrength));
    float expK = lerp(1.4, 2.6, strength);
    float atmospheric = (1.0 - exp(-screenV * expK)) / (1.0 - exp(-expK));
    float lum = WE_LogLerp(gBot, gTop, atmospheric);

    lum *= saturate(backgroundBrightness);
    lum = clamp(lum, kExposureFloor, kExposureCeiling);

    float3 color = float3(lum, lum, lum);

    // Blue-noise + IGN dither in 8-bit space to eliminate banding without visible grain.
    float2 pixel = input.position.xy;
    float dither = (WE_BlueNoise(pixel) + WE_InterleavedGradientNoise(pixel)) * 0.5 - 0.5;
    color += dither / 255.0;

    color = WE_ClampCharcoalExposure(color, kExposureCeiling, kExposureFloor);
    return float4(color, 1.0);
}
