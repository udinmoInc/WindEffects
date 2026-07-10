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

float4 PSMain(VSOutput input) : SV_Target
{
    float coverage = texAtlas.Sample(samp0, input.uv).a;
    float4 outColor = input.color;
    outColor.a *= coverage;
    return outColor;
}
