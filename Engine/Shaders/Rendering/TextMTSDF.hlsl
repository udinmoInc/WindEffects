struct VSInput
{
    [[vk::location(0)]] float2 position : POSITION0;
    [[vk::location(1)]] float2 uv       : TEXCOORD0;
    [[vk::location(2)]] float4 color    : COLOR0;
    [[vk::location(3)]] float4 params   : TEXCOORD1;
};

struct VSOutput
{
    float4 position                     : SV_Position;
    [[vk::location(0)]] float2 uv       : TEXCOORD0;
    [[vk::location(1)]] float4 color    : COLOR0;
};

struct TextPushConstants
{
    float2 uScale;
    float2 uTranslate;
    float2 uAtlasSize;
    float  uPixelRange;
};

[[vk::push_constant]]
TextPushConstants pc;

[[vk::binding(0, 0)]]
Texture2D    texAtlas : register(t0, space0);
SamplerState samp0    : register(s0, space0);

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.uv = input.uv;
    o.color = input.color;
    o.position = float4(input.position * pc.uScale + pc.uTranslate, 0.0, 1.0);
    return o;
}

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(float2 uv, float pxRange, float2 atlasSize)
{
    float2 unitRange = pxRange / atlasSize;
    float2 screenTexSize = 1.0 / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float4 texel = texAtlas.Sample(samp0, input.uv);
    float sd = median3(texel.r, texel.g, texel.b);
    float pxRange = screenPxRange(input.uv, pc.uPixelRange, pc.uAtlasSize);
    float rgbOpacity = saturate((sd - 0.5) * pxRange + 0.5);
    float4 outColor = input.color;
    outColor.a *= max(rgbOpacity, texel.a);
    return outColor;
}
