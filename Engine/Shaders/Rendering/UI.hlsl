struct VSInput
{
    float2 position : POSITION0;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR0;
    float4 sdfRect  : TEXCOORD1;
    float4 sdfParams: TEXCOORD2;
};

struct VSOutput
{
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float4 color     : COLOR0;
    float4 sdfRect   : TEXCOORD1;
    float4 sdfParams : TEXCOORD2;
    float2 worldPos  : TEXCOORD3;
};

struct UIPushConstants
{
    float2 uScale;
    float2 uTranslate;
};

[[vk::push_constant]]
UIPushConstants pc;

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.uv = input.uv;
    o.color = input.color;
    o.sdfRect = input.sdfRect;
    o.sdfParams = input.sdfParams;
    o.worldPos = input.position;
    o.position = float4(input.position * pc.uScale + pc.uTranslate, 0.0, 1.0);
    return o;
}

Texture2D    texSampler : register(t0, space0);
SamplerState samp0      : register(s0, space0);

float udRoundBox(float2 p, float2 b, float r)
{
    return length(max(abs(p) - b + r, 0.0)) - r;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float type = input.sdfParams.y;
    if (type < 0.5)
        return input.color * texSampler.Sample(samp0, input.uv);

    float2 center = float2(input.sdfRect.x + input.sdfRect.z * 0.5, input.sdfRect.y + input.sdfRect.w * 0.5);
    float2 halfSize = float2(input.sdfRect.z * 0.5, input.sdfRect.w * 0.5);
    float radius = input.sdfParams.x;
    float dist = udRoundBox(input.worldPos - center, halfSize, radius);
    float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);

    float4 outColor = input.color;
    outColor.a *= alpha;
    return outColor;
}
