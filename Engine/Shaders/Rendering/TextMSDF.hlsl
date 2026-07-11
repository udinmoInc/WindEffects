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
    o.sdfParams = input.sdfParams;
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
    float3 msdf = texAtlas.Sample(samp0, input.uv).rgb;
    float sd = median3(msdf.r, msdf.g, msdf.b);
    float pxRange = max(input.sdfParams.z, max(pc.uPixelRange, 1.0));
    float spr = screenPxRange(input.uv, pxRange, pc.uAtlasSize);
    float opacity = saturate((sd - 0.5) * spr + 0.5);
    float4 outColor = input.color;
    outColor.a *= opacity;
    return outColor;
}
