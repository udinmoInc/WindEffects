struct VSInput
{
    [[vk::location(0)]] float2 position  : POSITION0;
    [[vk::location(1)]] float2 uv        : TEXCOORD0;
    [[vk::location(2)]] float4 color     : COLOR0;
    [[vk::location(3)]] float4 sdfRect   : TEXCOORD1;
    [[vk::location(4)]] float4 sdfParams : TEXCOORD2;
};

struct VSOutput
{
    float4 position                      : SV_Position;
    [[vk::location(0)]] float2 uv        : TEXCOORD0;
    [[vk::location(1)]] float4 color     : COLOR0;
    [[vk::location(2)]] float4 sdfParams : TEXCOORD1;
};

struct TextPushConstants
{
    float2 uScale;
    float2 uTranslate;
    float2 uAtlasSize;
    float  uPixelRange;
    float  padding;
};

#if defined(WE_TARGET_DXIL)
cbuffer TextPushConstantBuffer : register(b0, space0)
{
    TextPushConstants pc;
};
#else
[[vk::push_constant]]
TextPushConstants pc;
#endif

[[vk::binding(0, 0)]]
Texture2D    texAtlas : register(t0, space0);
SamplerState samp0    : register(s0, space0);

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.uv = input.uv;
    o.color = input.color;
    o.sdfParams = input.sdfParams;
    o.position = float4(input.position * pc.uScale + pc.uTranslate, 0.0, 1.0);
    return o;
}

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

// Chlumsky MSDF screen-space range: atlas distance field range in texels,
// converted to screen pixels via UV derivatives (handles DPI/scale automatically).
float screenPxRange(float2 uv, float pxRange, float2 atlasSize)
{
    float2 unitRange = pxRange / max(atlasSize, float2(1.0, 1.0));
    float2 screenTexSize = 1.0 / max(fwidth(uv), float2(1e-5, 1e-5));
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float4 PSMain(VSOutput input) : SV_Target
{
    // Sample MSDF in linear atlas space (no sRGB decode on distance fields).
    float3 msdf = texAtlas.Sample(samp0, input.uv).rgb;
    float sd = median3(msdf.r, msdf.g, msdf.b);

    // sdfParams.z carries the atlas pixel range; push constant is the batch fallback.
    float pxRange = max(input.sdfParams.z, max(pc.uPixelRange, 1.0));
    float spr = screenPxRange(input.uv, pxRange, pc.uAtlasSize);

    // Center the threshold at 0.5 (MSDF mid-edge). spr scales AA to ~1 screen pixel.
    float opacity = saturate((sd - 0.5) * spr + 0.5);

    // Premultiplied-friendly output: keep vertex color in the working color space
    // of the UI pass (linear blend). Do not apply additional gamma here.
    float4 outColor = input.color;
    outColor.a *= opacity;
    return outColor;
}
